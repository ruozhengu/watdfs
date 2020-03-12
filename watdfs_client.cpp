//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"
#include "utils.h"
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
#include <dirent.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int readonly = O_RDONLY;

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *ret_code) {
    // set up the RPC library by calling `rpcClientInit`.
    int initRet = rpcClientInit();

    if (initRet == 0) DLOG("Client Init Success");
    else DLOG("Client Init Fail");

    *ret_code = initRet;

    // init global trackers
    openFiles *userdata=  new std::map<std::string, fileMetadata>(); //TODO: free it
    cacheInterval = cache_interval;
    cachePath = (char *)malloc(strlen(path_to_cache) + 1);
    strcpy(cachePath, path_to_cache);

    // Return pointer to global state data.
    return (void *) userdata;
}

//TODO: might need to free path as well
void watdfs_cli_destroy(void *userdata) {

    delete userdata;
    int destoryRet = rpcClientDestroy();

    if (!destoryRet) DLOG("Client Destory Success");
    else DLOG("Client Destory Fail");

    userdata = NULL;
}

// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL
    DLOG("NEW: Received getattr call from local client...");

    int fxn_ret = 0;
    int ret_code = 0;
    int sys_ret = 0;


    char *cache_path = get_cache_path(path);

    if (is_file_open((openFiles *)userdata), path) {

      DLOG("file opened before, checking freshness ...");

      // check client server consistency
      ret_code = freshness_check((openFiles *) userdata, path, 0);

      // handle return code
      if (ret_code < 0) {
        free(cache_path);
        return ret_code;
      }

      // set to current time since it is just opened
      struct fileMetadata * target = (*((openFiles *) userdata))[std::string(path)];
      target->tc = time(NULL); // curr time

      sys_ret = stat(cache_path, statbuf);
      if (sys_ret < 0) {
          memset(statbuf, 0, sizeof(struct stat));
          DLOG("getattr system call failed(1) ...");
          free(cache_path);
          fxn_ret = -errno;
          return fxn_ret;
      }
      fxn_ret = ret_code;

    } else {
      //if file is never opened before
      DLOG("first time opening the file, checking if it's on server ...");
      struct stat sb; // on stack

      // return error code if file exists on server ...
      if (-2 == rpcCall_getattr(userdata, path, tmp_statbuf) {

        // exsistence leads to an error ...
        DLOG("FAILED: file exists on server");
        fxn_ret = -2;
        free(cache_path);
        return fxn_ret;
      }

      struct fuse_file_info * fi = new struct fuse_file_info;
      fi->flags = O_RDONLY; // update flags

      ret_code = watdfs_cli_open(userdata, path, fi); // sys call
      if (ret_code < 0){
          delete fi;
          free(cache_path);
          return ret_code;
      }

      sys_ret = stat(cache_path, statbuf); // sys call

      if (sys_ret < 0) {
        memset(statbuf, 0, sizeof(struct stat));
        fxn_ret = -errno;
        delete fi;
        free(cache_path);
        return fxn_ret;
      }

      // release the file
      fxn_ret = watdfs_cli_release(userdata, path, fi);

      //TODO: delete fi?
    }

    free(cache_path); // eventually freed

    DLOG("SUCCESS: getattr: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {

    DLOG("NEW: Received fgetattr call from local client...");

    int fxn_ret = 0;
    int ret_code = 0;
    int sys_ret = 0;


    char *cache_path = get_cache_path(path);

    if (is_file_open((openFiles *)userdata), path) {

      DLOG("file opened before, checking freshness ...");

      // check client server consistency
      ret_code = freshness_check((openFiles *) userdata, path, 0);

      // handle return code
      if (ret_code < 0) {
        free(cache_path);
        return ret_code;
      }

      // set to current time since it is just opened
      struct fileMetadata * target = (*((openFiles *) userdata))[std::string(path)];
      target->tc = time(NULL); // curr time

      sys_ret = fstat(target->client_mode, statbuf);
      if (sys_ret < 0) {
          memset(statbuf, 0, sizeof(struct stat));
          DLOG("getattr system call failed(1) ...");
          free(cache_path);
          fxn_ret = -errno;
          return fxn_ret;
      }
      fxn_ret = ret_code;

    } else {
      //if file is never opened before
      DLOG("first time opening the file, checking if it's on server ...");
      struct stat sb; // on stack

      // return error code if file exists on server ...
      if (-2 == rpcCall_getattr(userdata, path, &sb) {

        // exsistence leads to an error ...
        DLOG("FAILED: file exists on server");
        fxn_ret = -2;
        free(cache_path);
        return fxn_ret;
      }

      struct fuse_file_info * fi = new struct fuse_file_info;
      fi->flags = O_RDWR; // update flags

      ret_code = rpcCall_open(userdata, path, fi); // sys call
      if (ret_code < 0){
          delete fi;
          free(cache_path);
          return ret_code;
      }
      struct fileMetadata * target = (*((openFiles *) userdata))[std::string(path)];
      sys_ret = fstat(target->client_mode, statbuf); // sys call

      if (sys_ret < 0) {
        memset(statbuf, 0, sizeof(struct stat));
        fxn_ret = -errno;
        delete fi;
        free(cache_path);
        return fxn_ret;
      }

      // close the file descriptor
      fxn_ret = close(target->client_mode);

      // remove closed file
      ((openFiles *)userdata)->erase(std::string(path));

      //TODO: delete fi?
    }

    free(cache_path); // eventually freed

    DLOG("SUCCESS: fgetattr: return code is %d", fxn_ret);
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {

    DLOG("Received mknod rpcCall from local client...");

    int ARG_COUNT = 4;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int arg_types[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // The second argument is the mode. This argument is an input
    // only argument, and we treat it as a integer.
    arg_types[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
    args[1] = (void *)&mode;

    // The third argument is dev, an input only argument, which is
    // an long integer type.
    arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
    args[2] = (void *)&dev;

    // The third argument is return code, an output only argument, which is
    // an integer type.
    arg_types[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    int ret_code = 0;
    args[3] = (void *)&ret_code;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[4] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"mknod", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        DLOG( "Mknod rpcCall: fail");
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) DLOG("Mknod rpcCall: return code is negative");

    char *cache_path = get_cache_path(path);

    // creates a file system node using mknod sys call
    int sys_ret = mknod(cache_path, mode, dev);

    if (sys_ret < 0) {
      DLOG("fail to create file ...");
      fxn_ret = -EINVAL;
    } else {
      fxn_ret = sys_ret;
    }

    // Clean up the memory we have allocated.
    free(args);
    free(full_path);

    DLOG("DONE: mknod: return code is %d", fxn_ret);

    return fxn_ret;
}


int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {
    // Called to open a file.
    // SET UP THE RPC CALL
    DLOG("Received open rpcCall from local client...");


    if (is_file_open((openFiles *)userdata), path) return EMFILE;

    // add to openFiles
    (*((openFiles *)userdata))[std::string(path)] = new struct fileMetadata;

    // update flags
    if (readonly == (fi->flags & O_ACCMODE)) fi->flags = O_RDONLY; // server
    else fi->flags = O_RDWR;


    // getattr has 4 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the output arguments.
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[ARG_COUNT + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // The second argument is the fi structure. This argument is an input/output
    // only argument, and we treat it as char array
    arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) |
          (ARG_CHAR << 16u) | (uint)sizeof(struct fuse_file_info);

    args[1] = (void *)fi;


    // The second argument is return code, an output only argument, which is
    // an integer type.
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    int ret_code = 0;
    args[2] = (void *)&ret_code;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"open", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        DLOG( "Open rpcCall: fail");
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        DLOG("OPEN: SUCCESSSSSS %d",ret_code);
        DLOG("OPEN: %ld", fi->fh);
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) DLOG("Open rpcCall: return code is negative");

    int old_flag = fi->flags;
    // update flags
    if (readonly == (fi->flags & O_ACCMODE)) fi->flags = O_RDWR; // client
    else fi->flags = O_RDWR;

    int ret_code;
    ret_code = download_to_client(userdata, path, fi);

    if (ret_code < 0) {
      fxn_ret = ret_code;
    } else {
      fxn_ret = 0;
    }

    // modify metadata
    (*((openFiles *) userdata))[std::string(path)]->tc = time(NULL);

    // set flag back
    fi->flags = old_flag;

    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: open: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path,
                       struct fuse_file_info *fi) {

    int sys_ret = 0;
    int ret_code = 0;
    int fxn_ret = 0;

    DLOG("Received release rpcCall from local client...");

    if (!(readonly == (fi->flags & O_ACCMODE))) {
      // not read only, push the updates to server
       sys_ret = push_to_server(userdata, path, fi);
       if (sys_ret < 0) return sys_ret;
    }

   ret_code = rpcCall_release(userdata, path, fi);

   if (ret_code < 0){
     fxn_ret = ret_code;
     return fxn_ret;
   }

   // now remove the files from openFiles and close it locally

    struct fileMetadata * target = (*((opened_files*)userdata))[std::string(path)];

    sys_ret = close(target->client_mode);

    if (sys_ret < 0) fxn_ret = -errno;
    else ((openFiles *)userdata)->erase(std::string(path));


    return sys_ret;
}

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {

    DLOG("Received read rpcCall from local client...");

    int fxn_ret = 0;

    int ret_code = freshness_check((openFiles *)userdata, path, 0);
    struct fileMetadata * target = (*((opened_files*)userdata))[std::string(path)];

    target->tc = time(NULL); // curr time

    if (ret_code < 0){
      fxn_ret = ret_code;
      DLOG("freshness check in read failed...");
      return fxn_ret;
    }

    fxn_ret = _read(userdata, path, buf, size, offset, fi);\
    if (fxn_ret < 0) return fxn_ret;
    return fxn_ret;
}


int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {

    DLOG("Received write rpcCall from local client...");
    int fxn_ret = 0;

    int sys_ret = _write(userdata, path, buf, size, offset, fi);
    if (sys_ret < 0) return sys_ret;


    int ret_code = freshness_check((openFiles *) userdata, path, 1);
    struct fileMetadata * target = (*((opened_files*)userdata))[std::string(path)];

    target->tc = time(NULL); // curr time

    if (ret_code < 0){
      fxn_ret = ret_code;
      DLOG("freshness check in write failed...");
      return fxn_ret;
    }
    return sys_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {

  DLOG("Received truncate rpcCall from local client...");

  int fxn_ret = 0;

  char *cache_path = get_cache_path(path);

  // call system call here
  sys_ret = truncate(cache_path, newsize);

  // check freshness
  fxn_ret = freshness_check((openFiles *) userdata, path, 1);

  if (ret_code < 0){
    DLOG("freshness check in truncate failed...");
    free(full_path);
    return fxn_ret;
  }
  free(full_path);

  return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {

  DLOG("Received fsync rpcCall from local client...");

  // // getattr has 4 arguments.
  // int ARG_COUNT = 3;
  //
  // // Allocate space for the output arguments.
  // void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
  //
  // // Allocate the space for arg types, and one extra space for the null
  // // array element.
  // int arg_types[ARG_COUNT + 1];
  //
  // // The path has string length (strlen) + 1 (for the null character).
  // int pathlen = strlen(path) + 1;
  //
  // // Fill in the arguments
  // // The first argument is the path, it is an input only argument, and a char
  // // array. The length of the array is the length of the path.
  // arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
  // args[0] = (void *)path;
  //
  // // The second argument is the fi. This argument is an input
  // // only argument, and we treat it as char array
  // arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
  //  (uint)sizeof(struct fuse_file_info);
  // args[1] = (void *)fi;
  //
  // // The second argument is return code, an output only argument, which is
  // // an integer type.
  // arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
  // int ret_code = 0;
  // args[2] = (void *)&ret_code;
  //
  // // Finally, the last position of the arg types is 0. There is no
  // // corresponding arg.
  // arg_types[3] = 0;
  //
  // // MAKE THE RPC CALL
  // int rpc_ret = rpcCall((char *)"fsync", arg_types, args);
  //
  // // HANDLE THE RETURN
  // int fxn_ret = 0;
  // if (rpc_ret < 0) {
  //   // Something went wrong with the rpcCall, return a sensible return
  //   // value. In this case lets return, -EINVAL
  //   DLOG( "Fsync rpcCall: fail");
  //   fxn_ret = -EINVAL;
  // } else {
  //   // Our RPC call succeeded. However, it's possible that the return code
  //   // from the server is not 0, that is it may be -errno. Therefore, we
  //   // should set our function return value to the retcode from the server.
  //   fxn_ret = ret_code;
  // }
  //
  // if (fxn_ret < 0) DLOG("Fsync rpcCall: return code is negative");
  //
  //
  // // Clean up the memory we have allocated.
  // free(args);

  int ret_code = 0;
  int fxn_ret = 0;
  ret_code = push_to_server(userdata, path, fi);

  if (ret_code < 0) {
    DLOG("fsyn failed to upload data");
    fxn_ret = ret_code;
  }

  //update time
  struct fileMetadata * target = (*((opened_files*)userdata))[std::string(path)];

  target->tc = time(NULL); // curr time

  DLOG("DONE: sync: return code is %d", fxn_ret);

  // Finally return the value we got from the server.
  return fxn_ret;
}

// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path,
     const struct timespec ts[2]) {

  // Called to release a file.
  // SET UP THE RPC CALL
  DLOG("Received utimens rpcCall from local client...");

  char *cache_path = get_cache_path(path);
  int ret_code = 0;
  // init new stat to update meta
  struct stat *statbuf = new struct stat;
  int fxn_ret = stat(full_path, statbuf);

  if (sys_ret < 0) {
    DLOG("error in utimens for stat sys call");
    fxn_ret = -errno;
    free(cache_path);
    memset(statbuf, 0, sizeof(struct stat));
    return fxn_ret;
  }

  // check atime and mtime consistency, if not need to check freshness
  bool is_atime_diff = ((statbuf->st_atim).tv_nsec != ts[0].tv_nsec) ||
                        ((statbuf->st_atim).tv_sec != ts[0].tv_sec);

  bool is_mtime_diff = ((statbuf->st_mtim).tv_nsec != ts[1].tv_nsec) ||
                        ((statbuf->st_mtim).tv_sec != ts[1].tv_sec);


  if (is_atime_diff){
    // need to check read freshnnes
    ret_code = freshness_check(userdata, path, 0);

    if (utils_ret < 0){
      DLOG("utimens freshness check failed");
      return utils_ret;
    }

    sys_ret = utimensat(0, cache_path, ts, O_RDONLY);

  }

  if (is_mtime_diff){
    // need to check write freshness
    ret_code = freshness_check(userdata, path, 1);

    if (utils_ret < 0){
      DLOG("utimens freshness check failed");
      return utils_ret;
    }

    sys_ret = utimensat(0, cache_path, ts, O_WRONLY);

  }
  // extra sys call to utimensat
  if (rw_flag == 1) {
    ret_code = rpcCall_utimens(userdata, path, ts);
    if (ret_code < 0){
      DLOG("utimens rpccall check failed");
      return ret_code;
    }
  }

  DLOG("DONE: truncate: return code is %d", fxn_ret);

  // Finally return the value we got from the server.
  return fxn_ret;
}
