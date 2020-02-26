//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "rpc.h"
#include "debug.h"
INIT_LOG

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <fuse.h>


// Global state server_persist_dir.
char *server_persist_dir = nullptr;

// Important: the server needs to handle multiple concurrent client requests.
// You have to be carefuly in handling global variables, esp. for updating them.
// Hint: use locks before you update any global variable.

// We need to operate on the path relative to the the server_persist_dir.
// This function returns a path that appends the given short path to the
// server_persist_dir. The character array is allocated on the heap, therefore
// it should be freed after use.
// Tip: update this function to return a unique_ptr for automatic memory management.
char *get_full_path(char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len = strlen(server_persist_dir);
    int full_len = dir_len + short_path_len + 1;

    char *full_path = (char *)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, server_persist_dir);
    // Then append the path.
    strcat(full_path, short_path);
    DLOG("Full path: %s\n", full_path);

    return full_path;
}

// The server implementation of getattr.
int watdfs_getattr(int *argTypes, void **args) {

    DLOG("start sys call: getattr");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat *)args[1];
    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // TODO: Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should use the statbuf as an argument to the stat system call.

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = stat(full_path, statbuf);

    if (sys_ret < 0) {
      // If there is an error on the system call, then the return code should
      // be -errno.
      *ret = -errno;
      DLOG("sys call: getattr failed");
    } else {
      DLOG("sys call: getattr succeed");
      *ret = 0; //should be 0
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_fgetattr(int *argTypes, void **args) {

    DLOG("start sys call: fgetattr");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat *)args[1];

    // The third argument is the fuse_file_info,a file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[2];

    // The fourth argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = fstat(fi->fh, statbuf);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: fgetattr failed");
    } else {
      *ret = 0;
      DLOG("sys call: fgetattr succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_open(int *argTypes, void **args) {

    DLOG("start sys call: open");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is the fuse_file_info,a file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = open(full_path, fi->flags);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: open failed");
    } else {
      fi->fh = sys_ret;
      DLOG("**** OPEN sys_ret is : %d", sys_ret);

      *ret= 0;
      DLOG("sys call: open succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_write(int *argTypes, void **args) {

    DLOG("start sys call: write");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is the buffer, to write to file
    const char *buf = (const char *)args[1];

    // The third argument is size, how many to write
    size_t *size = (size_t *)args[2];

    // The fourth argument is offset to write
    off_t *offset = (off_t *)args[3];

    // The fifth argument is fuse_file_info , which is file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];

    // The sixth argument is return code,
    int *ret = (int *)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = pwrite(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: write failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: write succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_read(int *argTypes, void **args) {

    DLOG("start sys call: read");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is the buffer, to write to file
    char *buf = (char *)args[1];

    // The third argument is size, how many to write
    size_t *size = (size_t *)args[2];

    // The fourth argument is offset to write
    off_t *offset = (off_t *)args[3];

    // The fifth argument is fuse_file_info , which is file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];

    // The sixth argument is return code,
    int *ret = (int *)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = pread(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: read failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: read succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_mknod(int *argTypes, void **args) {

    DLOG("start sys call: mknod");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is the mode, how to create file
    mode_t *mode = (mode_t *)args[1];

    // The third argument is dev, indicate if file is special
    dev_t *dev = (dev_t *)args[2];

    // The fourth argument is return code
    int *ret = (int *)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = mknod(full_path, *mode, *dev);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: mknod failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: mknod succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_fsync(int *argTypes, void **args) {

    DLOG("start sys call: fsync");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is fuse_file_info , which is file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is return code
    int *ret = (int*) args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = fsync(fi->fh);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: fsync failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: fsync succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_utimens(int *argTypes, void **args) {

    DLOG("start sys call: utimens");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is timespec
    struct timespec *ts = (struct timespec *)args[1];

    // The third argument is return code
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = utimensat(0, full_path, ts, 0);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: utimens failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: utimens succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_truncate(int *argTypes, void **args) {

    DLOG("start sys call: truncate");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is new size, to save
    off_t *size = (off_t *)args[1];

    // The third argument is return code,
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = truncate(full_path, *size);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: truncate failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: truncate succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

int watdfs_release(int *argTypes, void **args) {

    DLOG("start sys call: release");

    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];

    // The second argument is the fuse_file_info,a file descriptor
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];

    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;

    sys_ret = close(fi->fh);

    if (sys_ret < 0) {
      *ret = -errno;
      DLOG("sys call: close failed");
    } else {
      *ret = sys_ret;
      DLOG("sys call: close succeed");
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}

// The main function of the server.
int main(int argc, char *argv[]) {

    DLOG("Server init...");
    // argv[1] should contain the directory where you should store data on the
    // server. If it is not present it is an error, that we cannot recover from.
    if (argc != 2) {
        // In general you shouldn't print to stderr or stdout, but it may be
        // helpful here for debugging. Important: Make sure you turn off logging
        // prior to submission!
        // See watdfs_client.c for more details
        // # ifdef PRINT_ERR
        // std::cerr << "Usaage:" << argv[0] << " server_persist_dir";
        // #endif
        return -1;
    }
    // Store the directory in a global variable.
    server_persist_dir = argv[1];

    // TODO: Initialize the rpc library by calling `rpcServerInit`.
    // Important: `rpcServerInit` prints the 'export SERVER_ADDRESS' and
    // 'export SERVER_PORT' lines. Make sure you *do not* print anything
    // to *stdout* before calling `rpcServerInit`.
    //DLOG("Initializing server...");

    int ret_code = rpcServerInit();
    int ret = 0;

    if (!ret_code) {
      DLOG("Server Init Succeed ...");
    } else {
      DLOG("Server Init Failed ...");
      return ret_code;
    }
    // TODO: If there is an error with `rpcServerInit`, it maybe useful to have
    // debug-printing here, and then you should return.

    // TODO: Register your functions with the RPC library.
    // Note: The braces are used to limit the scope of `argTypes`, so that you can
    // reuse the variable for multiple registrations. Another way could be to
    // remove the braces and use `argTypes0`, `argTypes1`, etc.
    {
        // There are 3 args for the function (see watdfs_client.c for more
        // detail).
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "getattr", argTypes, watdfs_getattr);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: getattr fail ... ");
            return ret;
        }
        DLOG("Register: getattr succeed ... ");
    }
    // Register mknod
    {
        // There are 4 args for the function. (see watdfs_client.c for more
        // detail).

        int argTypes[5];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
        argTypes[2] =  (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[4] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "mknod", argTypes, watdfs_mknod);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: mknod fail ... ");
	          return ret;
        }
        DLOG("Register: mknod succeed ... ");
    }

    // Register fgetattr
    {
        // There are 4 args for the function.
        int argTypes[5];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[4] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "fgetattr", argTypes, watdfs_fgetattr);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: fgetattr fail ... ");
	          return ret;
        }
        DLOG("Register: fgetattr succeed ... ");
    }

    // Register open
    {
        // There are 3 args for the function (see watdfs_client.c for more
        // detail).
        int argTypes[4];
        // First is the path.
        argTypes[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_INPUT) | (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u ;
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        ret = rpcRegister((char *) "open", argTypes, watdfs_open);
        if (ret < 0) {
            DLOG("Register: open fail ... ");
            return ret;
        }
        DLOG("Register: open succeed ... ");
    }

    // Register release
    {
        // There are 3 args for the function.
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "release", argTypes, watdfs_release);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: release fail ... ");
	          return ret;
        }
        DLOG("Register: release succeed ... ");
    }

    // Register write
    {
        // There are 6 args for the function.
        int argTypes[7];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[4] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "write", argTypes, watdfs_write);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: write fail ... ");
	          return ret;
        }
        DLOG("Register: write succeed ... ");
    }

    // Register read
    {
        // There are 5 args for the function.
        int argTypes[7];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[4] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "read", argTypes, watdfs_read);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: read fail ... ");
	          return ret;
        }
        DLOG("Register: read succeed ... ");
    }

    // Register truncate
    {
        // There are 3 args for the function.
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "truncate", argTypes, watdfs_truncate);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: truncate fail ... ");
	          return ret;
        }
        DLOG("Register: truncate succeed ... ");
    }

    // Register fsync
    {
        // There are 3 args for the function.
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "fsync", argTypes, watdfs_fsync);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: fsync fail ... ");
	          return ret;
        }
        DLOG("Register: fsync succeed ... ");
    }

    // Register utimens
    {
        // There are 3 args for the function.
        int argTypes[4];
        // First is the path.
        argTypes[0] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[1] =
            (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
        argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char *) "utimens", argTypes, watdfs_utimens);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            DLOG("Register: utimens fail ... ");
	          return ret;
        }
        DLOG("Register: utimens succeed ... ");
    }

    // TODO: Hand over control to the RPC library by calling `rpcExecute`.
    ret_code = rpcExecute();

    // handle error
    if(!ret_code){
        DLOG("Executing server succeed...");
    } else{
        DLOG("Executing server Fail...");
    }
    // rpcExecute could fail so you may want to have debug-printing here, and
    // then you should return.
    return ret;
}
