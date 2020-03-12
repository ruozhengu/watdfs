//
// Starter code for CS 454
// You SHOULD change this file
//
//

#include "rpc.h"
#include "rw_lock.h"

// You may need to change your includes depending on whether you use C or C++.

// Needed for stat.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fuse.h>
#include <fcntl.h>
#include<iostream>


#define ELOCK -666
#define EUNLOCK -667


// Needed for errors.
#include <errno.h>

// Needed for string operations.
#include <cstring>

// Need malloc and free.
#include <cstdlib>

#include <map> // for recording the opened files

// You may want to include iostream or cstdio.h if you print to standard error.


// a3
struct file_metadata{
    int state; //  {0: read, 1: write}
    rw_lock_t* lock ;
};


std::map<std::string, struct file_metadata *> openedFilesStates;


// Global state server_persist_dir.
char *server_persist_dir = NULL;

// We need to operate on the path relative to the the server_persist_dir.
// This function returns a path that appends the given short path to the
// server_persist_dir. The character array is allocated on the heap, therefore
// it should be freed after use.
char* get_full_path(char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len = strlen(server_persist_dir);
    int full_len = dir_len + short_path_len + 1;

    char *full_path = (char*)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, server_persist_dir);
    // Then append the path.
    strcat(full_path, short_path);

    return full_path;
}

// The server implementation of getattr.
int watdfs_getattr(int *argTypes, void **args) {
    std::cerr <<"server getattr called"<< std::endl;
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char*)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat*)args[1];
    // The third argument is the return code, which will be 0, or -errno.
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should make the stat system call here:
    // Let sys_ret be the return code from the stat system call.
    int sys_ret = 0;
    // You should use the statbuf as an argument to the stat system call, but it
    // is currently unused.
    sys_ret = stat(full_path, statbuf);

    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_fgetattr(int *argTypes, void **args) {
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char*)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat*)args[1];
    // The 3rd argument is the fi
    struct fuse_file_info *fi = (struct fuse_file_info *) args[2];
    // The 4th argument is the return code, which will be 0, or -errno.
    int *ret = (int*)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = fstat(fi->fh, statbuf);

    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    // The RPC call should always succeed, so return 0.
    return 0;
}


int watdfs_mknod(int *argTypes, void **args)
{
    std::cerr <<"mknod called"<< std::endl;
    char *short_path = (char*)args[0];
    mode_t * mode = (mode_t *) args[1];
    dev_t * dev = (dev_t *) args[2];
    int *ret = (int*)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the open system call
    int sys_ret = 0;
# ifdef PRINT_ERR
    std::cerr <<"mknod"<< std::endl;
    std::cerr << "mode " << *mode << " dev " << *dev << std::endl;
    std::cerr << "path " << full_path << std::endl;
#endif

    sys_ret = mknod(full_path, *mode, *dev);
    if (sys_ret < 0) {
# ifdef PRINT_ERR
        std::cerr <<"mknod system call failed" << std::endl;
#endif
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
# ifdef PRINT_ERR
        std::cerr <<"mknod"<< std::endl;
        std::cerr << "ret code:" << ret << std::endl;
#endif
        *ret = sys_ret; // open function returns file descripter
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

// ************ open not set the file descriptor ************
int watdfs_open(int *argTypes, void **args) {
    std::cerr <<"***open called"<< std::endl;
    char *short_path = (char*)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *) args[1];
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the open system call
    int sys_ret = 0;

    std::cerr <<"****O_RDONLY "<< O_RDONLY << "O_RDWR " << O_RDWR << std::endl;

    // we should assume file exists on the server because of FUSE mknod
    if (openedFilesStates.find(short_path) == openedFilesStates.end()){//file has not been opened
        std::cerr <<"***** not opened flag " << fi->flags << std::endl;
        openedFilesStates[short_path] = new struct file_metadata;
        openedFilesStates[short_path]->lock = new rw_lock_t;
        rw_lock_init(openedFilesStates[short_path]->lock);
        if (fi->flags == O_RDONLY){
            openedFilesStates[short_path]->state = 0;// 0 for read
        }else{
            std::cerr <<"***** write on server " << fi->flags <<  std::endl;
            openedFilesStates[short_path]->state = 1;// 1 for RDWR
        }

    }else{// the file has already been opened
        int state = openedFilesStates[short_path]->state;
        std::cerr <<"***** opened flag " << fi->flags << " state " << state <<  std::endl;

        if (state == 0 && fi->flags == O_RDWR){
            openedFilesStates[short_path]->state = 1;
            std::cerr <<"***** opened flag " << fi->flags << " state " << openedFilesStates[short_path]->state <<  std::endl;
        }else{
            std::cerr <<"***** opened flag " << fi->flags << " state " << state <<  std::endl;
            if (state == 1 && fi->flags == O_RDWR){
                std::cerr <<"!!!!! ***** confliction " <<  std::endl;
                return EACCES;
            }
        }
    }

    sys_ret = open(full_path, fi->flags);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        fi->fh = sys_ret;
    }

    std::cerr <<"open finished "<< sys_ret << std::endl;
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_release(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *) args[1];
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = close(fi->fh);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_read(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    char *buf = (char*)args[1];
    size_t * size = (size_t *)args[2];
    off_t * offset = (off_t *)(args[3]);
    struct fuse_file_info *fi = (struct fuse_file_info *) args[4];
    int *ret = (int*)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = pread(fi->fh, buf, *size, *offset);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        *ret = sys_ret;//return the number of bytes read/write
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_write(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    char *buf = (char*)args[1];
    size_t * size = (size_t *)args[2];
    off_t * offset = (off_t *)args[3];
    struct fuse_file_info *fi = (struct fuse_file_info *) args[4];
    int *ret = (int*)args[5];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    std::cerr << "* server fh " << fi->fh << std::endl;
    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = pwrite(fi->fh, buf, *size, *offset);

    std::cerr << "* server write finished with " << sys_ret << std::endl;

    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        *ret = sys_ret;//return the number of bytes read/write
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_truncate(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    off_t newsize = *((off_t *)args[1]);
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = truncate(full_path, newsize);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        *ret = sys_ret;
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_fsync(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *) args[1];
    int *ret = (int*)args[2];

    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = fsync(fi->fh);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        *ret = sys_ret;//no idea
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_utimens(int *argTypes, void **args) {
    char *short_path = (char*)args[0];
    struct timespec * ts = (struct timespec *) args[1];
    int *ret = (int*)args[2];

    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the stat system call
    int sys_ret = 0;

    sys_ret = utimensat(0, full_path, ts, 0);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }else{
        *ret = sys_ret;//no idea
    }
    // Clean up the full path, it was allocated on the heap.
    free(full_path);
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_lock(int *argTypes, void **args){
    std::cerr <<"lock called"<< std::endl;
    char *short_path = (char*)args[0];
    rw_lock_mode_t * lockmode = (rw_lock_mode_t *) args[1];
    int *ret = (int*)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the open system call
    int sys_ret = 0;

    std::cerr <<"path "<< short_path << " lock mode " << *lockmode  << std::endl;

    sys_ret = rw_lock_lock(openedFilesStates[short_path]->lock, *lockmode);
    if (sys_ret < 0) {
        *ret = ELOCK;
    }else{
        *ret = sys_ret; // open function returns file descripter
    }
    free(full_path);
    std::cerr <<"lock finished "<< sys_ret <<  std::endl;
    // The RPC call should always succeed, so return 0.
    return 0;
}

int watdfs_unlock(int *argTypes, void **args){
    std::cerr <<"unlock called"<< std::endl;
    char *short_path = (char*)args[0];
    rw_lock_mode_t * lockmode = (rw_lock_mode_t *) args[1];
    int *ret = (int*)args[2];

    // Initially we set set the return code to be 0.
    *ret = 0;

    // Make the open system call
    int sys_ret = 0;

    std::cerr <<"path "<< short_path << " lock mode " << *lockmode  << std::endl;

    sys_ret = rw_lock_unlock(openedFilesStates[short_path]->lock, *lockmode);
    if (sys_ret < 0) {
        *ret = EUNLOCK;
    }else{
        *ret = sys_ret; // open function returns file descripter
    }
    // The RPC call should always succeed, so return 0.
    std::cerr <<"unlock finished "<< sys_ret << std::endl;
    return 0;
}


// The main function of the server.
int main(int argc, char *argv[]) {
    // argv[1] should contain the directory where you should store data on the
    // server. If it is not present it is an error, that we cannot recover from.
    if (argc != 2) {
        // In general you shouldn't print to stderr or stdout, but it may be
        // helpful here for debugging. Important: Make sure you turn off logging
        // prior to submission!
        // See watdfs_client.c for more details
# ifdef PRINT_ERR
        std::cerr << "Usage:" << argv[0] << " server_persist_dir" << std::endl;
#endif
        return -1;
    }
    // Store the directory in a global variable.
    server_persist_dir = argv[1];

    // Initialize the rpc library by calling rpcServerInit. You should call
    // rpcServerInit here:
    int ret = 0;

    //*********** atomic file transfer ***********

    ret = rpcServerInit();
    std::cerr << "rpcServer Initialized:" << ret << std::endl;

    // If there is an error with rpcServerInit, it maybe useful to have
    // debug-printing here, and then you should return.
# ifdef PRINT_ERR
    std::cerr << "rpcServerInit Failed, code:" << ret << std::endl;
#endif
    // Register your functions with the RPC library.
    {// attr
        // There are 3 args for the function (see watdfs_client.c for more detail).
        int argTypes[4];
        // First is the path.
        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;
        // Note for arrays we can set the length to be anything  > 1.

        // The second argument is the statbuf.
        argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;
        // The third argument is the retcode.
        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
        // Finally we fill in the null terminator.
        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"getattr", argTypes, watdfs_getattr);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //fgetattr
        // array length all set to 1
        int num_args = 4;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1; // statbuf

        argTypes[2] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; //

        argTypes[4] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"fgetattr", argTypes, watdfs_fgetattr);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //mknod
        // array length all set to 1
        int num_args = 4;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_INPUT) |  (ARG_INT << 16) ; // mode

        argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ; // not array, last 2 bytes 0

        argTypes[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[4] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"mknod", argTypes, watdfs_mknod);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //open
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_OUTPUT) |(1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"open", argTypes, watdfs_open);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //release
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"release", argTypes, watdfs_release);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //read
        // array length all set to 1
        int num_args = 6;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;

        argTypes[3] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;

        argTypes[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        argTypes[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"read", argTypes, watdfs_read);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //write
        // array length all set to 1
        int num_args = 6;
        int argTypes[num_args+1];

        // The first argument
        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        // The second argument
        argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        // The 3rd argument
        argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;

        // The 4th argument
        argTypes[3] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;

        // The 5th argument
        argTypes[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | 1;

        // The 6th argument
        argTypes[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        // Last argument
        argTypes[6] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"write", argTypes, watdfs_write);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //truncate
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        // The first argument
        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        // The second argument
        argTypes[1] = (1 << ARG_INPUT) | (ARG_LONG << 16);

        // The 3rd argument
        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"truncate", argTypes, watdfs_truncate);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //fsync
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        // The first argument is the path
        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        // fi
        argTypes[1] = (1 << ARG_INPUT) |(1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        // return code
        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"fsync", argTypes, watdfs_fsync);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //utimens
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)| (ARG_CHAR << 16) | 1;

        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"utimens", argTypes, watdfs_utimens);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //lock
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16) ;

        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"lock", argTypes, watdfs_lock);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    { //unlock
        // array length all set to 1
        int num_args = 3;
        int argTypes[num_args+1];

        argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;

        argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16) ;

        argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;

        argTypes[3] = 0;

        // We need to register the function with the types and the name.
        ret = rpcRegister((char*)"unlock", argTypes, watdfs_unlock);
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }


    // Hand over control to the RPC library by calling rpcExecute. You should call
    // rpcExecute here:
    ret = rpcExecute();

    // rpcExecute could fail so you may want to have debug-printing here, and then
    // you should return.
    if (ret < 0) {
# ifdef PRINT_ERR
        std::cerr << "Usaage:" << argv[0] << " server_persist_dir" << std::endl;
#endif
    }

    // iterating over all value of opened_files
    std::map<std::string, struct file_metadata *>:: iterator itr;
    for (itr = openedFilesStates.begin(); itr != openedFilesStates.end(); itr++){
        //itr->second->file_statistics;
        rw_lock_t* lock = itr->second->lock;
        rw_lock_destroy(lock);
        delete itr->second;
    }

    return ret;
}

// //
// // Starter code for CS 454/654
// // You SHOULD change this file
// //
//
// #include "rpc.h"
// #include "debug.h"
// INIT_LOG
//
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <unistd.h>
// #include <errno.h>
// #include <cstring>
// #include <cstdlib>
// #include <fuse.h>
//
//
// // Global state server_persist_dir.
// char *server_persist_dir = nullptr;
//
// // Important: the server needs to handle multiple concurrent client requests.
// // You have to be carefuly in handling global variables, esp. for updating them.
// // Hint: use locks before you update any global variable.
//
// // We need to operate on the path relative to the the server_persist_dir.
// // This function returns a path that appends the given short path to the
// // server_persist_dir. The character array is allocated on the heap, therefore
// // it should be freed after use.
// // Tip: update this function to return a unique_ptr for automatic memory management.
// char *get_full_path(char *short_path) {
//     int short_path_len = strlen(short_path);
//     int dir_len = strlen(server_persist_dir);
//     int full_len = dir_len + short_path_len + 1;
//
//     char *full_path = (char *)malloc(full_len);
//
//     // First fill in the directory.
//     strcpy(full_path, server_persist_dir);
//     // Then append the path.
//     strcat(full_path, short_path);
//     DLOG("Full path: %s\n", full_path);
//
//     return full_path;
// }
//
// // The server implementation of getattr.
// int watdfs_getattr(int *argTypes, void **args) {
//
//     DLOG("start sys call: getattr");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//     // The second argument is the stat structure, which should be filled in
//     // by this function.
//     struct stat *statbuf = (struct stat *)args[1];
//     // The third argument is the return code, which should be set be 0 or -errno.
//     int *ret = (int *)args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // TODO: Make the stat system call, which is the corresponding system call needed
//     // to support getattr. You should use the statbuf as an argument to the stat system call.
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = stat(full_path, statbuf);
//
//     if (sys_ret < 0) {
//       // If there is an error on the system call, then the return code should
//       // be -errno.
//       *ret = -errno;
//       DLOG("sys call: getattr failed");
//     } else {
//       DLOG("sys call: getattr succeed");
//       *ret = 0; //should be 0
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_fgetattr(int *argTypes, void **args) {
//
//     DLOG("start sys call: fgetattr");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//     // The second argument is the stat structure, which should be filled in
//     // by this function.
//     struct stat *statbuf = (struct stat *)args[1];
//
//     // The third argument is the fuse_file_info,a file descriptor
//     struct fuse_file_info *fi = (struct fuse_file_info *)args[2];
//
//     // The fourth argument is the return code, which should be set be 0 or -errno.
//     int *ret = (int *)args[3];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = fstat(fi->fh, statbuf);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: fgetattr failed");
//     } else {
//       *ret = 0;
//       DLOG("sys call: fgetattr succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_open(int *argTypes, void **args) {
//
//     DLOG("start sys call: open");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is the fuse_file_info,a file descriptor
//     struct fuse_file_info *fi = (struct fuse_file_info *)args[1];
//
//     // The third argument is the return code, which should be set be 0 or -errno.
//     int *ret = (int *)args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = open(full_path, fi->flags);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: open failed");
//     } else {
//       fi->fh = sys_ret;
//       DLOG("**** OPEN sys_ret is : %d", sys_ret);
//
//       *ret= 0;
//       DLOG("sys call: open succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_write(int *argTypes, void **args) {
//
//     DLOG("start sys call: write");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is the buffer, to write to file
//     const void *buf = args[1];
//
//     // The third argument is size, how many to write
//     size_t *size = (size_t *)args[2];
//
//     // The fourth argument is offset to write
//     off_t *offset = (off_t *)args[3];
//
//     // The fifth argument is fuse_file_info , which is file descriptor
//     struct fuse_file_info *fi = (struct fuse_file_info *)args[4];
//
//     // The sixth argument is return code,
//     int *ret = (int *)args[5];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = pwrite(fi->fh, buf, *size, *offset);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: write failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: write succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//
//     return *ret;
// }
//
// int watdfs_read(int *argTypes, void **args) {
//
//     DLOG("start sys call: read");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is the buffer, to write to file
//     char *buf = (char *)args[1];
//
//     // The third argument is size, how many to write
//     size_t *size = (size_t *)args[2];
//
//     // The fourth argument is offset to write
//     off_t *offset = (off_t *)args[3];
//
//     // The fifth argument is fuse_file_info , which is file descriptor
//     fuse_file_info *fi = (struct fuse_file_info *)args[4];
//
//     // The sixth argument is return code,
//     int *ret = (int *)args[5];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = pread(fi->fh, buf, *size, *offset);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: read failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: read succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return *ret;
// }
//
// int watdfs_mknod(int *argTypes, void **args) {
//
//     DLOG("start sys call: mknod");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is the mode, how to create file
//     mode_t *mode = (mode_t *)args[1];
//
//     // The third argument is dev, indicate if file is special
//     dev_t *dev = (dev_t *)args[2];
//
//     // The fourth argument is return code
//     int *ret = (int *)args[3];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = mknod(full_path, *mode, *dev);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: mknod failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: mknod succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_fsync(int *argTypes, void **args) {
//
//     DLOG("start sys call: fsync");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is fuse_file_info , which is file descriptor
//     struct fuse_file_info *fi = (struct fuse_file_info *)args[1];
//
//     // The third argument is return code
//     int *ret = (int*) args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = fsync(fi->fh);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: fsync failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: fsync succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_utimens(int *argTypes, void **args) {
//
//     DLOG("start sys call: utimens");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is timespec
//     struct timespec *ts = (struct timespec *)args[1];
//
//     // The third argument is return code
//     int *ret = (int*)args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = utimensat(0, full_path, ts, 0);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: utimens failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: utimens succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_truncate(int *argTypes, void **args) {
//
//     DLOG("start sys call: truncate");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is new size, to save
//     off_t *size = (off_t *)args[1];
//
//     // The third argument is return code,
//     int *ret = (int *)args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = truncate(full_path, *size);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: truncate failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: truncate succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_release(int *argTypes, void **args) {
//
//     DLOG("start sys call: release");
//
//     // Get the arguments.
//     // The first argument is the path relative to the mountpoint.
//     char *short_path = (char *)args[0];
//
//     // The second argument is the fuse_file_info,a file descriptor
//     struct fuse_file_info *fi = (struct fuse_file_info *)args[1];
//
//     // The third argument is the return code, which should be set be 0 or -errno.
//     int *ret = (int *)args[2];
//
//     // Get the local file name, so we call our helper function which appends
//     // the server_persist_dir to the given path.
//     char *full_path = get_full_path(short_path);
//
//     // Initially we set set the return code to be 0.
//     *ret = 0;
//
//     // Let sys_ret be the return code from the stat system call.
//     int sys_ret = 0;
//
//     sys_ret = close(fi->fh);
//
//     if (sys_ret < 0) {
//       *ret = -errno;
//       DLOG("sys call: close failed");
//     } else {
//       *ret = sys_ret;
//       DLOG("sys call: close succeed");
//     }
//
//     // Clean up the full path, it was allocated on the heap.
//     free(full_path);
//     DLOG("Returning code: %d", *ret);
//     // The RPC call succeeded, so return 0.
//     return 0;
// }
//
// int watdfs_unlock(int *argTypes, void **args){
//
//     char *short_path = (char *) args[0];
//     rw_lock_mode_t * lockmode = (rw_lock_mode_t *) args[1];
//     int *ret_code = (int *) args[2];
//
//     *ret_code = 0; // update args2[2] = 0
//
//     // Make the open system call
//     int sys_ret = 0;
//
//     sys_ret = rw_lock_unlock(openedFilesStates[short_path]->lock, *lockmode);
//     if (sys_ret < 0) {
//         *ret = EUNLOCK;
//     }else{
//         *ret = sys_ret; // open function returns file descripter
//     }
//     // The RPC call should always succeed, so return 0.
//     std::cerr <<"unlock finished "<< sys_ret << std::endl;
//     return 0;
// }
//
// // The main function of the server.
// int main(int argc, char *argv[]) {
//
//     DLOG("Server init...");
//     // argv[1] should contain the directory where you should store data on the
//     // server. If it is not present it is an error, that we cannot recover from.
//     if (argc != 2) {
//         // In general you shouldn't print to stderr or stdout, but it may be
//         // helpful here for debugging. Important: Make sure you turn off logging
//         // prior to submission!
//         // See watdfs_client.c for more details
//         // # ifdef PRINT_ERR
//         // std::cerr << "Usaage:" << argv[0] << " server_persist_dir";
//         // #endif
//         return -1;
//     }
//     // Store the directory in a global variable.
//     server_persist_dir = argv[1];
//
//     // TODO: Initialize the rpc library by calling `rpcServerInit`.
//     // Important: `rpcServerInit` prints the 'export SERVER_ADDRESS' and
//     // 'export SERVER_PORT' lines. Make sure you *do not* print anything
//     // to *stdout* before calling `rpcServerInit`.
//     //DLOG("Initializing server...");
//
//     int ret_code = rpcServerInit();
//     int ret = 0;
//
//     if (!ret_code) {
//       DLOG("Server Init Succeed ...");
//     } else {
//       DLOG("Server Init Failed ...");
//       return ret_code;
//     }
//     // TODO: If there is an error with `rpcServerInit`, it maybe useful to have
//     // debug-printing here, and then you should return.
//
//     // TODO: Register your functions with the RPC library.
//     // Note: The braces are used to limit the scope of `argTypes`, so that you can
//     // reuse the variable for multiple registrations. Another way could be to
//     // remove the braces and use `argTypes0`, `argTypes1`, etc.
//     {
//         // There are 3 args for the function (see watdfs_client.c for more
//         // detail).
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "getattr", argTypes, watdfs_getattr);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: getattr fail ... ");
//             return ret;
//         }
//         DLOG("Register: getattr succeed ... ");
//     }
//     // Register mknod
//     {
//         // There are 4 args for the function. (see watdfs_client.c for more
//         // detail).
//
//         int argTypes[5];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] = (1u << ARG_INPUT) | (ARG_INT << 16u);
//         argTypes[2] =  (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[4] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "mknod", argTypes, watdfs_mknod);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: mknod fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: mknod succeed ... ");
//     }
//
//     // Register fgetattr
//     {
//         // There are 4 args for the function.
//         int argTypes[5];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[4] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "fgetattr", argTypes, watdfs_fgetattr);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: fgetattr fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: fgetattr succeed ... ");
//     }
//
//     // Register open
//     {
//         // There are 3 args for the function (see watdfs_client.c for more
//         // detail).
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_INPUT) | (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u ;
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         ret = rpcRegister((char *) "open", argTypes, watdfs_open);
//         if (ret < 0) {
//             DLOG("Register: open fail ... ");
//             return ret;
//         }
//         DLOG("Register: open succeed ... ");
//     }
//
//     // Register release
//     {
//         // There are 3 args for the function.
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "release", argTypes, watdfs_release);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: release fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: release succeed ... ");
//     }
//
//     // Register write
//     {
//         // There are 6 args for the function.
//         int argTypes[7];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[4] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[6] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "write", argTypes, watdfs_write);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: write fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: write succeed ... ");
//     }
//
//     // Register read
//     {
//         // There are 5 args for the function.
//         int argTypes[7];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[4] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[6] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "read", argTypes, watdfs_read);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: read fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: read succeed ... ");
//     }
//
//     // Register truncate
//     {
//         // There are 3 args for the function.
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "truncate", argTypes, watdfs_truncate);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: truncate fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: truncate succeed ... ");
//     }
//
//     // Register fsync
//     {
//         // There are 3 args for the function.
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "fsync", argTypes, watdfs_fsync);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: fsync fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: fsync succeed ... ");
//     }
//
//     // Register utimens
//     {
//         // There are 3 args for the function.
//         int argTypes[4];
//         // First is the path.
//         argTypes[0] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[1] =
//             (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
//         argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char *) "utimens", argTypes, watdfs_utimens);
//         if (ret < 0) {
//             // It may be useful to have debug-printing here.
//             DLOG("Register: utimens fail ... ");
// 	          return ret;
//         }
//         DLOG("Register: utimens succeed ... ");
//     }
//
//     { //lock
//
//         int num_args = 3;
//         int argTypes[num_args + 1];
//
//         argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;
//         argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16) ;
//         argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char*)"lock", argTypes, watdfs_lock);
//         if (ret < 0) return ret;
//
//     }
//
//     { //unlock
//
//         int num_args = 3;
//         int argTypes[num_args + 1];
//
//         argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 1;
//         argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16) ;
//         argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;
//         argTypes[3] = 0;
//
//         // We need to register the function with the types and the name.
//         ret = rpcRegister((char*)"unlock", argTypes, watdfs_unlock);
//         if (ret < 0) return ret;
//
//     }
//
//     // TODO: Hand over control to the RPC library by calling `rpcExecute`.
//     ret_code = rpcExecute();
//
//     // handle error
//     if(!ret_code){
//         DLOG("Executing server succeed...");
//     } else{
//         DLOG("Executing server Fail...");
//     }
//     // rpcExecute could fail so you may want to have debug-printing here, and
//     // then you should return.
//     return ret;
// }
