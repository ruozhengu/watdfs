#include "watdfs_client.h"
#include "rpc.h"

// You may want to include iostream or cstdio.h if you print to standard error.
#include "printer.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
using namespace std;

// Key: Path | Value: pair(fd,fi)
map<string, pair<int, struct fuse_file_info *> > openfiles;

// Global state local_persist_dir.
const char *_path_to_cache;
time_t t;
time_t Tc;

// Forward declartion
int rpc_getattr(void *userdata, const char *path, struct stat *statbuf);
int rpc_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                      struct fuse_file_info *fi);
int rpc_mknod(void *userdata, const char *path, mode_t mode, dev_t dev);
int rpc_open(void *userdata, const char *path, struct fuse_file_info *fi);
int rpc_release(void *userdata, const char *path, struct fuse_file_info *fi);
int rpc_read(void *userdata, const char *path, char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi);
int rpc_write(void *userdata, const char *path, const char *buf,
                   size_t size, off_t offset, struct fuse_file_info *fi);
int rpc_fsync(void *userdata, const char *path,
                   struct fuse_file_info *fi);
int rpc_truncate(void *userdata, const char *path, off_t newsize);
int rpc_utimens(void *userdata, const char *path,
                     const struct timespec ts[2]);


/* ------------------------- HELPERS ---------------------------- */
char* get_full_cache_path(const char *short_path) {
  int short_path_len = strlen(short_path);
  int dir_len = strlen(_path_to_cache);
  int full_len = dir_len + short_path_len + 1;

  char *full_path = (char*)malloc(full_len);

  // First fill in the directory.
  strcpy(full_path, _path_to_cache);
  // Then append the path.
  strcat(full_path, short_path);

  return full_path;
}

bool is_fresh(void *userdata, const char *path) {
  bool ret = false;
  char *local_path = get_full_cache_path( path );

  if ((time(0) - Tc) < t) {
    ret = true;
    Tc = time(0);
  } else {
    struct stat *statbuf_cli = (struct stat *) malloc(sizeof(struct stat));
    int getattr_cli = stat( local_path, statbuf_cli );

    struct stat *statbuf_server = (struct stat *) malloc(sizeof(struct stat));
    int getattr_server = rpc_getattr( userdata, path, statbuf_server );

    if (statbuf_cli->st_mtime == statbuf_server->st_mtime) {
      ret = true;
      Tc = time(0);
    }

    free(statbuf_cli);
    free(statbuf_server);
  }

  free(local_path);

  return ret;
}

bool is_opened(string path) {
  if (openfiles.find(path) != openfiles.end()) {
    return true;
  } else {
    return false;
  }
}

int get_fd(string path) {
  return openfiles[path].first;
}

fuse_file_info *get_fi(string path) {
  return openfiles[path].second;
}
/* ------------------------------------------------------------ */


/* ------------------------- TRANSFER ---------------------------- */
int watdfs_transfer_file_to_client(void *userdata, const char *path,
                                struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );

  struct stat *statbuf_server = (struct stat *) malloc(sizeof(struct stat));
  int getattr_server = rpc_getattr( userdata, path, statbuf_server );

  int fd_cli = open( local_path, O_RDWR );
  // If no cached copy
  if (fd_cli < 0) {
    // Create local file
    int mknod_cli = mknod( local_path, statbuf_server->st_mode, statbuf_server->st_dev );
    if (mknod_cli == 0) {
      // Open local file again
      fd_cli = open( local_path, O_RDWR );
    } else {
      print("ERROR: (watdfs_transfer_file_to_client) mknod returned ", mknod_cli);
    }
  }

  // Truncate the file as same as the one in server
  int truncate_cli = truncate( local_path, statbuf_server->st_size );
  if (truncate_cli < 0) {
    print("ERROR: (watdfs_transfer_file_to_client) truncate returned ", truncate_cli);
  }

  // Read from server
  char *buf = (char*) malloc(statbuf_server->st_size * sizeof(char));
  int read_server = rpc_read( userdata, path, buf, statbuf_server->st_size, 0, fi );
  if (read_server < 0) {
    print("ERROR: (watdfs_transfer_file_to_client) rpc_read returned ", read_server);
  }

  // Write to local file
  int write_cli = pwrite( fd_cli, buf, statbuf_server->st_size, 0 );
  if (write_cli < 0) {
    print("ERROR: (watdfs_transfer_file_to_client) pwrite returned ", write_cli);
  }

  // Update metadata
  struct stat *statbuf_cli = (struct stat *) malloc(sizeof(struct stat));
  int getattr_cli = stat( local_path, statbuf_cli );
  // Update client statbuf with server statbuf
  statbuf_cli->st_mtime = statbuf_server->st_mtime;
  statbuf_cli->st_ctime = statbuf_server->st_ctime;

  // After transfer from server, reset Tc
  Tc = time(0);

  free(buf);
  free(statbuf_server);
  free(statbuf_cli);
  free(local_path);

  return 0;
}

int watdfs_transfer_file_to_server(void *userdata, const char *path,
                                struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );

  struct stat *statbuf_cli = (struct stat *) malloc(sizeof(struct stat));
  int getattr_cli = stat( local_path, statbuf_cli );

  // Truncate the file as same as the one in local
  int truncate_server = rpc_truncate( userdata, path, statbuf_cli->st_size );
  if (truncate_server < 0) {
    print("ERROR: (watdfs_transfer_file_to_server) rpc_truncate returned ", truncate_server);
  }

  // Open local file
  int fd_cli = open( local_path, O_RDONLY );

  // Read from local
  char *buf = (char*) malloc(statbuf_cli->st_size * sizeof(char));
  int read_cli = pread( fd_cli, buf, statbuf_cli->st_size, 0 );
  if (read_cli < 0) {
    print("ERROR: (watdfs_transfer_file_to_server) pread returned ", read_cli);
  }

  // Write to server file
  int write_server = rpc_write( userdata, path, buf, statbuf_cli->st_size, 0, fi );
  if (write_server < 0) {
    print("ERROR: (watdfs_transfer_file_to_server) rpc_write returned ", write_server);
  }

  // Update metadata
  struct stat *statbuf_server = (struct stat *) malloc(sizeof(struct stat));
  int getattr_server = rpc_getattr( userdata, path, statbuf_server );
  // Update client statbuf with server statbuf
  statbuf_server->st_mtime = statbuf_cli->st_mtime;
  statbuf_server->st_ctime = statbuf_cli->st_ctime;

  // After transfer to server, reset Tc
  Tc = time(0);

  free(buf);
  free(statbuf_cli);
  free(statbuf_server);
  free(local_path);

  return 0;
}
/* ------------------------------------------------------------ */


/* ------------------------- SETUP ---------------------------- */
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                    time_t cache_interval) {
  // You should set up the RPC library here, by calling rpcClientInit.
  // You should check the return code of the rpcClientInit call, as it may fail,
  // for example, if the incorrect port was exported. If there was an error,
  // it may be useful to print to stderr or stdout during debugging.
  // Important: Make sure you turn off logging prior to submission!
  // One useful technique is to use pre-processor flags like:
  // # ifdef PRINT_ERR
  // fprintf(stderr, "Failed to initialize RPC Client\n");
  // Or if you prefer c++:
  // std::cerr << "Failed to initialize RPC Client";
  // #endif
  int rpc_ret = rpcClientInit();
  if (rpc_ret < 0) {
    print("FAIL: initialize RPC Client with code: ", rpc_ret);
  }

  // You can also initialize any global state that you want to have in this
  // method, and return it. The value that you return here will be passed
  // as userdata in other functions.
  _path_to_cache = path_to_cache;
  t = cache_interval;

  // path_to_cache and cache_interval are not necessary for Assignment 2, but should
  // be used in Assignment 3.
  return NULL;
}
void watdfs_cli_destroy(void *userdata) {
  // You should clean up your userdata state here.
  // You should also tear down the RPC library by calling rpcClientDestroy.
  free(userdata);
  int rpc_ret = rpcClientDestroy();
  if (rpc_ret < 0) {
    print("FAIL: destroy RPC Client with code: ", rpc_ret);
  }
}
/* ------------------------------------------------------------ */


/* ------------------------- GETATTR ---------------------------- */
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
  struct fuse_file_info *fi = (struct fuse_file_info *) malloc(sizeof(struct fuse_file_info));

  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int transfer_cli, fxn_ret = 0;

  if (!is_opened(str_local_path)) {
    int getattr_server = rpc_getattr( userdata, path, statbuf );
    // file doesn't exist in server
    if (getattr_server < 0) {
      fxn_ret = getattr_server;
      print("ERROR: (watdfs_cli_getattr) rpc_getattr returned ", getattr_server);
    } else {
      // Transfer the file to client
      transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
    }
  } else {
    fi = get_fi( str_local_path );

    // Check freshness
    if(!is_fresh(userdata, path)){
      transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
    }
  }

  // Perform operation
  int getattr_cli = stat( local_path, statbuf );
  if (getattr_cli < 0) {
    memset(statbuf, 0, sizeof(struct stat));
    fxn_ret = -errno;
  } else {
    fxn_ret = getattr_cli;
  }

  // Close server
  int close_server = rpc_release( userdata, path, fi );
  if (close_server < 0) {
    print("ERROR: (watdfs_cli_getattr) rpc_release returned ", close_server);
  }

  // Clean up the memory we have allocated.
  free(local_path);

  return fxn_ret;
}
int rpc_getattr(void *userdata, const char *path, struct stat *statbuf) {
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct stat); // statbuf
  args[1] = (void*)statbuf;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  // MAKE THE RPC CALL
  int rpc_ret = rpcCall((char *)"getattr", arg_types, args);

  // HANDLE THE RETURN

  // The integer value watdfs_cli_getattr will return.
  int fxn_ret = 0;
  if (rpc_ret < 0) {
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  if (fxn_ret < 0) {
    memset(statbuf, 0, sizeof(struct stat));
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  // Finally return the value we got from the server.
  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* ------------------------- FGETATTR ----------------------------- */
int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                      struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int transfer_cli, fxn_ret = 0;

  if (!is_opened(str_local_path)) {
    int fgetattr_server = rpc_fgetattr( userdata, path, statbuf, fi );
    if (fgetattr_server < 0) {
      fxn_ret = -EINVAL;
    } else {
      transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
    }
  } else {
    // Check freshness
    if(!is_fresh(userdata, path)){
      transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
    }
  }

  // Perform operation
  int fd_cli = get_fd( str_local_path );

  int fgetattr_cli = fstat( fd_cli, statbuf );
  if (fgetattr_cli < 0) {
    memset(statbuf, 0, sizeof(struct stat));
    fxn_ret = -errno;
  } else {
    fxn_ret = fgetattr_cli;
  }

  // Close server
  int close_server = rpc_release( userdata, path, fi );
  if (close_server < 0) {
    print("ERROR: (watdfs_cli_getattr) rpc_release returned ", close_server);
  }

  free(local_path);

  return fxn_ret;
}
int rpc_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                      struct fuse_file_info *fi) {
  int num_args = 4;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(4 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct stat);
  args[1] = (void*)statbuf;

  arg_types[2] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[2] = (void*)fi;

  arg_types[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[3] = (void*)ret;

  arg_types[4] = 0;

  int rpc_ret = rpcCall((char *)"fgetattr", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[3];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* ------------------------- MKNOD ------------------------------ */
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
  struct fuse_file_info *fi = (struct fuse_file_info *) malloc(sizeof(struct fuse_file_info));
  struct stat *statbuf_server = (struct stat *) malloc(sizeof(struct stat));

  char *local_path = get_full_cache_path( path );

  int fxn_ret = 0;

  int getattr_server = rpc_getattr( userdata, path, statbuf_server );

  if (getattr_server < 0) {
    int mknod_server = rpc_mknod( userdata, path, mode, dev );
    if (mknod_server < 0) {
      print("ERROR: (watdfs_cli_mknod) rpc_mknod returned ", mknod_server);
    }

    int mknod_cli = mknod( local_path, mode, dev );
    if (mknod_cli < 0) {
      print("ERROR: (watdfs_cli_mknod) mknod returned ", mknod_cli);
    }

    free(fi);
  } else {
    int fd_server = rpc_open( userdata, path, fi );
    if (fd_server < 0) {
      print("ERROR: (watdfs_cli_mknod) rpc_open returned ", fd_server);
    }

    int transfer_server = watdfs_transfer_file_to_client( userdata, path, fi );

    // Close server
    int close_server = rpc_release( userdata, path, fi );
    if (close_server < 0) {
      print("ERROR: (watdfs_cli_mknod) rpc_release returned ", close_server);
    }
  }

  free(local_path);
  free(statbuf_server);

  return fxn_ret;
}
int rpc_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
  int num_args = 4;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(4 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
  args[1] = (void*)&mode;

  arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[2] = (void*)&dev;

  arg_types[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[3] = (void*)ret;

  arg_types[4] = 0;

  int rpc_ret = rpcCall((char *)"mknod", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[3];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* -------------------------- OPEN ------------------------------ */
int watdfs_cli_open(void *userdata, const char *path, struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int fxn_ret = 0;

   // file is not open
  if (!is_opened(str_local_path)) {
    // Open the file in server
    int fd_server = rpc_open( userdata, path, fi );
    if (fd_server < 0) {
      print("ERROR: (watdfs_cli_open) rpc_open returned ", fd_server);
    }

    // Open local(cache) copy for read write
    int fd_cli = open( local_path, fi->flags );
    if (fd_cli < 0) {
      fxn_ret = -errno;
    }

    // Transfer file to client
    int transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );

    // Add into global structure
    openfiles[str_local_path] = make_pair(fd_cli,fi);
  } else {
    fxn_ret = -EMFILE;
  }

  free(local_path);

  return fxn_ret;
}
int rpc_open(void *userdata, const char *path, struct fuse_file_info *fi) {
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[1] = (void*)fi;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  int rpc_ret = rpcCall((char *)"open", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* -------------------------- RELEASE ----------------------------- */
int watdfs_cli_release(void *userdata, const char *path, struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int transfer_server = watdfs_transfer_file_to_server( userdata, path, fi );

  int fxn_ret = 0;

  int fd_cli = get_fd( str_local_path );
  // Close local file
  int close_cli = close( fd_cli );
  if (close_cli < 0) {
    print("ERROR: (watdfs_cli_release) close returned ", close_cli);
  }

  // Close server
  int close_server = rpc_release( userdata, path, fi );
  if (close_server < 0) {
    fxn_ret = close_server;
    print("ERROR: (watdfs_cli_release) rpc_release returned ", close_server);
  }

  // Delete fd from global structure
  openfiles.erase(str_local_path);

  free(local_path);

  return fxn_ret;
}
int rpc_release(void *userdata, const char *path, struct fuse_file_info *fi) {
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[1] = (void*)fi;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  int rpc_ret = rpcCall((char *)"release", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* ------------------------ READ ------------------------------ */
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int total_bytes = 0;

  if(!is_fresh(userdata, path)) {
    int transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
  }

  int fd_cli = get_fd( str_local_path );
  int read_cli = pread( fd_cli, buf, size, offset );
  if (read_cli < 0) {
    total_bytes = -errno;
  } else {
    total_bytes = read_cli;
  }

  free(local_path);

  return total_bytes;
}
int rpc_read(void *userdata, const char *path, char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi) {
  // Read size amount of data at offset of file into buf.

  // Remember that size may be greater then the maximum array size of the RPC
  // library.
  int num_args = 6;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(6 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size;
  args[1] = (void*)buf;

  arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[2] = (void*)&size;

  arg_types[3] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[3] = (void*)&offset;

  arg_types[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[4] = (void*)fi;

  arg_types[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[5] = (void*)ret;

  arg_types[6] = 0;

  int total_bytes = 0;

  int rpc_ret;

  if (size > MAX_ARRAY_LEN) {
    size_t size_rem = size;

    size = MAX_ARRAY_LEN;
    arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size;

    args[2] = (void*)&size;

    do {
      if (size_rem < MAX_ARRAY_LEN) {
        arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size_rem;
        args[2] = (void*)&size_rem;

        rpc_ret = rpcCall((char *)"read", arg_types, args);

        total_bytes += *(int*)args[5];

        buf += *(int*)args[5]; // update buffer
        args[1] = (void*)buf;

        offset += *(int*)args[5]; // update offset
        args[3] = (void*)&offset;

        break;
      }
      rpc_ret = rpcCall((char *)"read", arg_types, args);

      total_bytes += *(int*)args[5];

      buf += *(int*)args[5]; // update buffer
      args[1] = (void*)buf;

      offset += *(int*)args[5]; // update offset
      args[3] = (void*)&offset;

      size_rem -= *(int *)args[5];
    } while(*(int *)args[5] > 0);
  } else {
    rpc_ret = rpcCall((char *)"read", arg_types, args);
    total_bytes = *(int*)args[5];
  }

  int fxn_ret;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -errno;
  } else {
    fxn_ret = total_bytes;
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* ------------------------- WRITE ------------------------------ */
int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                   size_t size, off_t offset, struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int total_bytes = 0;

  if(!is_fresh(userdata, path)) {
    int transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
  }

  int fd_cli = get_fd( str_local_path );
  int write_cli = pwrite( fd_cli, buf, size, offset );
  if (write_cli < 0) {
    total_bytes = -errno;
  } else {
    total_bytes = write_cli;
  }

  if(!is_fresh(userdata, path)) {
    int transfer_server = watdfs_transfer_file_to_server( userdata, path, fi );
  }

  free(local_path);

  return total_bytes;
}
int rpc_write(void *userdata, const char *path, const char *buf,
                   size_t size, off_t offset, struct fuse_file_info *fi) {
  // Write size amount of data at offset of file from buf.

  // Remember that size may be greater then the maximum array size of the RPC
  // library.
  int num_args = 6;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(6 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size;
  args[1] = (void*)buf;

  arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[2] = (void*)&size;

  arg_types[3] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[3] = (void*)&offset;

  arg_types[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[4] = (void*)fi;

  arg_types[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[5] = (void*)ret;

  arg_types[6] = 0;

  int total_bytes = 0;

  int rpc_ret;
  if (size > MAX_ARRAY_LEN) {
    size_t size_rem = size;

    size = MAX_ARRAY_LEN;
    arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size;

    args[2] = (void*)&size;

    do {
      if (size_rem < MAX_ARRAY_LEN) {
        arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | size_rem;
        args[2] = (void*)&size_rem;

        rpc_ret = rpcCall((char *)"write", arg_types, args);

        total_bytes += *(int*)args[5];

        buf += *(int*)args[5]; // update buffer
        args[1] = (void*)buf;

        offset += *(int*)args[5]; // update offset
        args[3] = (void*)&offset;

        break;
      }
      rpc_ret = rpcCall((char *)"write", arg_types, args);

      total_bytes += *(int*)args[5];

      buf += *(int*)args[5]; // update buffer
      args[1] = (void*)buf;

      offset += *(int*)args[5]; // update offset
      args[3] = (void*)&offset;

      size_rem -= *(int *)args[5];
    } while(*(int *)args[5] > 0);
  } else {
    rpc_ret = rpcCall((char *)"write", arg_types, args);
    total_bytes = *(int*)args[5];
  }

  int fxn_ret;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -errno;
  } else {
    fxn_ret = total_bytes;
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;

}
/* ------------------------------------------------------------ */


/* ------------------------- TRUNCATE --------------------------- */
int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {
  struct fuse_file_info *fi = (struct fuse_file_info *) malloc(sizeof(struct fuse_file_info));

  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int transfer_cli, fxn_ret = 0;

  // file is not open
  if (!is_opened(str_local_path)) {
    fi->flags = O_RDWR;
    transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
  }
  else {
    fi = get_fi( str_local_path );

    // Check freshness
    if(!is_fresh(userdata, path)){
      transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
    }
  }

  // Perform operation
  int truncate_cli = truncate( local_path, newsize );
  if (truncate_cli < 0) {
    fxn_ret = -errno;
    print("ERROR: (watdfs_cli_truncate) truncate returned ", truncate_cli);
  }

  // Check freshness
  if(!is_fresh(userdata, path)){
    int transfer_server = watdfs_transfer_file_to_server( userdata, path, fi );
  }

  // Close the file in server
  int close_server = rpc_release( userdata, path, fi );
  if (close_server < 0) {
    print("ERROR: (watdfs_cli_truncate) rpc_release returned ", close_server);
  }

  free(local_path);

  return fxn_ret;
}
int rpc_truncate(void *userdata, const char *path, off_t newsize) {
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (ARG_LONG << 16);
  args[1] = (void*)&newsize;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  int rpc_ret = rpcCall((char *)"truncate", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */


/* ------------------------- FSYNC ------------------------------ */
int watdfs_cli_fsync(void *userdata, const char *path,
                   struct fuse_file_info *fi) {
  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int fd_cli = get_fd( str_local_path );

  int fsync_cli = fsync( fd_cli );
  if (fsync_cli < 0) {
    print("ERROR: (watdfs_cli_fsync) fsync returned ", fsync_cli);
  }

  int transfer_server = watdfs_transfer_file_to_server( userdata, path, fi );

  free(local_path);

  return 0;
}
int rpc_fsync(void *userdata, const char *path,
                   struct fuse_file_info *fi) {
  // Force a flush of file data.
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
  args[1] = (void*)fi;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  int rpc_ret = rpcCall((char *)"fsync", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
/* ------------------------------------------------------------ */

// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path,
                     const struct timespec ts[2]) {
  struct fuse_file_info *fi = (struct fuse_file_info *) malloc(sizeof(struct fuse_file_info));

  char *local_path = get_full_cache_path( path );
  string str_local_path(local_path);

  int fxn_ret = 0;

  fi = get_fi( str_local_path );
  // Check freshness
  if(!is_fresh(userdata, path)){
    int transfer_cli = watdfs_transfer_file_to_client( userdata, path, fi );
  }

  int utimens_cli = utimensat( 0, local_path, ts, 0 );
  if (utimens_cli < 0) {
    fxn_ret = -EINVAL;
    print("ERROR: (watdfs_cli_utimens) utimesat returned ", utimens_cli);
  } else {
    fxn_ret = utimens_cli;
  }

  int utimens_server = rpc_utimens( userdata, path, ts );
  if (utimens_server < 0) {
    print("ERROR: (watdfs_cli_utimens) rpc_utimens returned ", utimens_server);
  }

  free(local_path);

  return fxn_ret;
}
int rpc_utimens(void *userdata, const char *path,
                     const struct timespec ts[2]) {
  // Change file access and modification times.
  int num_args = 3;

  // Allocate space for the output arguments.
  void **args = (void**) malloc(3 * sizeof(void*));

  // Allocate the space for arg types, and one extra space for the null
  // array element.
  int arg_types[num_args + 1];

  // The path has string length (strlen) + 1 (for the null character).
  int pathlen = strlen(path) + 1;

  int *ret = (int*) malloc(sizeof(int *));

  // Fill in the arguments
  arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
  args[0] = (void*)path;

  arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct timespec[2]);
  args[1] = (void*)ts;

  arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
  args[2] = (void*)ret;

  arg_types[3] = 0;

  int rpc_ret = rpcCall((char *)"utimens", arg_types, args);

  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return value.
    fxn_ret = -EINVAL;
  } else {
    fxn_ret = *(int*)args[2];
  }

  // Clean up the memory we have allocated.
  free(args);
  free(ret);

  return fxn_ret;
}
