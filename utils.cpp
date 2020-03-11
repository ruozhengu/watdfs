//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"
#include "watdfs_client_direct_access.cpp"

#include "debug.h"
INIT_LOG
#include <math.h>
#include "rw_lock.h"
#include "rpc.h"
#include <iostream>
#include <map>
#include <string>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>


// ------------ define util global var below ----------------------

// // global chache states to be init
// time_t cacheInterval;
// char *cachePath;

// track descriptor and last access time of each file
struct fileMetadata {
  int client_mode;
  int server_mode;
  time_t tc;
}

// track opened files by clients, just a type; key is not full path!
// typedef std::map<std::string, struct fileMetadata *> openFiles;


// ------------ define locks below ----------------------

static int lock(const char *path, int mode) {

    int ARG_COUNT = 3;
    int arg_types[ARG_COUNT + 1];
    void **args = (void**)malloc( ARG_COUNT*sizeof(void*));
    int pathlen = strlen(path) + 1;

    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) |
                    (ARG_CHAR << 16) | pathlen;
    args[0] = (void *) path;

    arg_types[1] = (1 << ARG_INPUT) |  (ARG_INT << 16) ;
    args[1] = (void *) &mode;

    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;

    int ret_code;
    args[2] = (void *) &ret_code;

    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"lock", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;

    if (rpc_ret < 0) {
      DOLG("lock itself rpc call failed")
      fxn_ret = -EINVAL;
      free(args);
      return fxn_ret;
    }

    fxn_ret = ret_code;
    free(args);
    return fxn_ret;
}

static int unlock(const char *path, int mode){

    int ARG_COUNT = 3;
    int arg_types[ARG_COUNT + 1];
    void **args = (void**)malloc( ARG_COUNT*sizeof(void*));

    int pathlen = strlen(path) + 1;

    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void*) path;

    arg_types[1] = (1 << ARG_INPUT) |  (ARG_INT << 16) ; // mode
    args[1] = (void*) &mode;

    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"unlock", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
      fxn_ret = -EINVAL;
      free(args);
      return fxn_ret;
    }

    fxn_ret = ret_code;
    free(args);
    return fxn_ret;
}


// ------------ define util functions below ----------------------

char *get_cache_path(const char* rela_path) {
  int rela_path_len = strlen(rela_path);
  int dir_len = strlen(cachePath);

  int full_path_len = dir_len + rela_path_len + 1;

  char *full_path = (char *) malloc(full_path_len);

  strcpy(full_path, cachePath);
  strcat(full_path, rela_path);

  return full_path;
}

bool is_file_open(openFiles *open_files, const char *path) {
  string p = std::string(path);
  return open_files->count(p) > 0 ? true : false;
}

struct fileMetadata * get_file_metadata(openFiles *open_files, const char *path) {
  return (*open_files)[std::string(path)];
}

// to be called in download function
// to write from server to local
static int _write(void *userdata, const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
  (void *) userdata;
  int sys_ret = pwrite(fi->fh, buf, size, offset);

  // handle error
  if (sys_ret < 0) {
    DLOG("write in download failed!");
    sys_ret = -errno;
  }
  return sys_ret;
}

// to be called in upload function
// to read from local to server
static int read_(void *userdata, const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {

    int sys_ret = pread(fi->fh, buf, size, offset);

    // handle error
    if (sys_ret < 0) {
      DLOG("write in download failed!");
      sys_ret = -errno;
    }
    return sys_ret;
}


static int download_to_client(void *userdata, const char *path, struct fuse_file_info *fi){

    DLOG("Running download function...");

    int sys_ret = lock(path, RW_READ_LOCK);

    if (sys_ret < 0){

      DLOG("fail to acquire the lock...");
      return sys_ret;

    } else {

      DLOG("got the lock...");

      int fxn_ret = sys_ret;

      struct stat statbuf; // on stack so no need to delete

      int rpc_ret = rpcCall_getattr(userdata, path, &statbuf);

      if (rpc_ret < 0){
          unlock(path, RW_READ_LOCK);
          return rpc_ret;
      }

      LOG("Download: getattr step good...");

      char *cache_path = get_cache_path(path);

      time_t server_last_open = statbuf.st_mtime;

      size_t size = statbuf.st_size;

      // open local file
      sys_ret = open(cache_path, O_CREAT | O_RDWR, S_IRWXU);

      if (sys_ret < 0) {
        LOG("FAIL: open local file");
        free(cache_path);
        unlock(path, RW_READ_LOCK);
        return -errno;
      }

      int local_fh_retcode = sys_ret;
      struct fileMetadata *target = (*((openFiles *) userdata))[path];
      target->client_mode = local_fh_retcode;

      // step1: truncate local file
      sys_ret = truncate(cache_path, (off_t)size);

      if (sys_ret < 0){
          free(cache_path);
          unlock(path, RW_READ_LOCK);
          return -errno;
      }

      LOG("Download: truncation step good...");

      // read the file from server
      char *buf = (char *) malloc(((off_t) size) * sizeof(char));

      rpc_ret = rpcCall_read(userdata, path, buf, size, 0, fi);

      if (rpc_ret < 0){
          free(buf);
          free(cache_path);
          unlock(path, RW_READ_LOCK);
          return rpc_ret;
      }

      LOG("Download: read step good...");

      // write from server to local
      sys_ret = _write(userdata, path, buf, size, 0, fi); //TODO: not sure

      if (sys_ret < 0){
        unlock(path, RW_READ_LOCK);
        free(buffer);
        free(cache_path);
        return sys_ret;
      }

      LOG("Download: write step good...");

      // update metadata
      target->client_mode = local_fh_retcode;// server
      target->server_mode = fi->fh;// local

      // update Tclient = Tserver and Tc = current time
      struct timespec *ts = new struct timespec[2];

      ts[0] = statbuf.st_atim;
      ts[1] = statbuf.st_mtim;

      int dirfd = 0;
      int flag = 0;
      // update the timestamps of a file by calling utimensat
      sys_ret = utimensat(dirfd, cache_path, ts, flag);

      LOG("Download: change metadata's step good...");


      sys_ret = unlock(path, RW_READ_LOCK);

      if (sys_ret < 0){
          free(buf);
          free(cache_path);
          delete ts;
          return sys_ret;
      }

      LOG("Download lock released ...");


      free(buf);
      free(cache_path);


      fxn_ret = sys_ret;
      return fxn_ret;
    }
}

static int push_to_server(void *userdata, const char *path, struct fuse_file_info *fi){

    DLOG("Running upload function...");

    int sys_ret = lock(path, RW_WRITE_LOCK);

    if (sys_ret < 0){

      DLOG("fail to acquire the lock...");
      return sys_ret;

    } else {

      DLOG("got the lock...");

      int fxn_ret = sys_ret;

      struct stat statbuf; // on stack so no need to delete
      char *cache_path = get_cache_path(path);
      sys_ret = stat(cache_path, &statbuf);

      if (sys_ret < 0) {
          memset(statbuf, 0, sizeof(struct stat));
          return sys_ret;
      }
      fxn_ret = sys_ret;

      time_t server_last_open = statbuf.st_mtime;
      size_t size = statbuf.st_size;
      char *buf = (char *) malloc(((off_t) size) * sizeof(char));

      // step1: truncate local file
      sys_ret = rpcCall_truncate(userdata, path, (off_t) size);

      if (sys_ret < 0){
          free(cache_path);
          free(buf);
          unlock(path, RW_WRITE_LOCK);
          return sys_ret;
      }

      LOG("Upload: truncation step good...");

      // read the file from client

      rpc_ret = _read(userdata, path, buf, size, 0, fi);

      if (rpc_ret < 0){
          free(buf);
          free(cache_path);
          unlock(path, RW_WRITE_LOCK);
          return rpc_ret;
      }

      LOG("Upload: read step good...");

      // write from server to local
      sys_ret = rpcCall_write(userdata, path, buf, size, 0, fi);

      if (sys_ret < 0){
        unlock(path, RW_WRITE_LOCK);
        free(buffer);
        free(cache_path);
        return sys_ret;
      }

      fxn_ret = sys_ret;

      LOG("Upload: write step good...");


      // update Tclient = Tserver and Tc = current time
      struct timespec *ts = new struct timespec[2];

      ts[0] = statbuf.st_atim;
      ts[1] = statbuf.st_mtim;


      // update the timestamps of a file by calling utimensat
      sys_ret = rpcCall_utimens(userdata, path, ts);

      fxn_ret = sys_ret;

      if (sys_ret < 0) {
        free(cache_path);
        free(buf);
        unlock(path, RW_WRITE_LOCK);
        return fxn_ret;
      }

      LOG("Upload: utimens step good...");

      sys_ret = unlock(path, RW_WRITE_LOCK);

      if (sys_ret < 0){
          free(buf);
          free(cache_path);
          delete ts;
          return sys_ret;
      }
      LOG("Upload lock released ...");


      free(buf);
      free(cache_path);

      fxn_ret = sys_ret;
      return fxn_ret;
    }
}

// w =1, r = 0
bool freshness_check(openFiles *open_files, const char *cache_path, const char *path, int rw_flag) {

  DLOG("Check Read Freshness", cache_path);

  int sys_ret = 0;
  int fxn_ret = 0;
  int rpc_ret = 0;

  struct fileMetadata * fm = get_file_metadata(open_file, path);
  struct stat * statbuf = new struct stat;
  time_t Tc = fm->tc;
  time_t T = time(0)

  sys_ret = stat(cache_path, statbuf);

  if (sys_ret < 0) {
    memset(statbuf, 0, sizeof(struct stat));
    return sys_ret;
  }

  time_t T_client = statbuf->st_mtim;

  fxn_ret = sys_ret;

  rpc_ret = rpcCall_getattr(userdata, path, statbuf);

  if (rpc_ret < 0){
    memset(statbuf, 0, sizeof(struct stat));
    return sys_ret;
  }

  time_t T_server = statbuf->st_mtim;

  struct fuse_file_info * fi = new struct fuse_file_info;

  bool is_time_within_interval = difftime(cacheInterval, difftime(T, Tc)) > 0;
  bool is_client_server_diff = (difftime(T_client, T_server)) != 0;

  if (!is_time_within_interval || !is_client_server_diff) {
    fi->fh = fm->server_mode;


    if (rw_flag == 0 ) { //read flag on
      fi->flags = O_RDONLY;
      rpc_ret = download_to_client(userdata, path, fi);
    } else {
      fi->flags = O_RDWR;
      rpc_ret = upload(userdata, path, fi);
    }

    if (rpc_ret < 0) {
      delete fi;
      memset(statbuf, 0, sizeof(struct stat));
    }
  } else {
    delete fi;
  }

  return fxn_ret;
}
