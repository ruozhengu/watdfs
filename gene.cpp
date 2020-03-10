//
// Starter code for CS 454
// You SHOULD change this file
//
//

#include "watdfs_client.h"

#include "rpc.h"
#include<iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#define PRINT_ERR

// You may want to include iostream or cstdio.h if you print to standard error.


// ret_code describes whether a syscall is successful or not
// the return from rpcCall is the desired output, like fh

// SETUP AND TEARDOWN

// GET FILE ATTRIBUTES
int watdfs_cli_getattr_remote(void *userdata, const char *path, struct stat *statbuf) {

    // SET UP THE RPC CALL
    // getattr has 3 arguments.
    int num_args = 3;

    // Allocate space for the output arguments.
    void **args = (void**) malloc(3 * sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path, it is an input only argument, and a char
    // array. The length of the array is the length of the path.
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The second argument is the stat structure. This argument is an output
    // only argument, and we treat it as a char array. The length of the array
    // is the size of the stat structure, which we can determine with sizeof.
    arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct stat); // statbuf
    args[1] = (void*)statbuf;

    // The third argument is the return code, an output only argument, which is
    // an integer. You should fill in this argument type here:
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0

    // The return code is not an array, so we need to hand args[2] an int*.
    // The int* could be the address of an integer located on the stack, or use
    // a heap allocated integer, in which case it should be freed.
    // You should fill in the argument here:
    int ret_code; //*************** return code on stack *******************
    args[2] = (void*) &ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"getattr", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {// rpc_ret denotes whether the rpcCall has succeeded or not
        // Something went wrong with the rpcCall, return a sensible return value.
        // In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we should
        // set our function return value to the ret_code from the server.
        // You should set the function return variable to the return code from the
        // server here:
# ifdef PRINT_ERR
        std::cerr <<"Remote getattr succeeded"<< std::endl;
        std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;// ret_code is the result of the getattr system call
    }

    if (fxn_ret < 0) {
        // Important: if the return code of watdfs_cli_getattr is negative (an
        // error), then we need to make sure that the stat structure is filled with
        // 0s. Otherwise, FUSE will be confused by the contradicting return values.
        memset(statbuf, 0, sizeof(struct stat));
    }

    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_fgetattr_remote(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {
    // SET UP THE RPC CALL

    // getattr has 4 arguments.
    int num_args = 4;
    void **args = (void**) malloc(num_args * sizeof(void*));

    int arg_types[num_args + 1];

    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The second argument is the stat structure.
    arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct stat); // statbuf
    args[1] = (void*)statbuf;

    // The 3rd arguement is fi
    arg_types[2] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void*) fi;

    // The 4th argument is the return code
    arg_types[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    // The return code is not an array, so we need to hand args[3] an int*.
    int ret_code; //*************** return code on stack *******************
    args[3] = (void*) &ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[4] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"fgetattr", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {
# ifdef PRINT_ERR
        std::cerr <<"Failed to getattr"<< std::endl;
        std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) {
        memset(statbuf, 0, sizeof(struct stat));
    }
    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod_remote(void *userdata, const char *path, mode_t mode, dev_t dev) {
    //called to create a file

    // SET UP THE RPC CALL
    // mknod has 4 arguments.
    int num_args = 4;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // 1st path
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void*) path;

    // The second argument
    arg_types[1] = (1 << ARG_INPUT) |  (ARG_INT << 16) ; // mode
    args[1] = (void*) &mode;

    // The third argument
    arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ; // not array, last 2 bytes 0
    args[2] = (void*) &dev;

    // The 4th argument
    arg_types[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[3] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[4] = 0;

# ifdef PRINT_ERR
    std::cerr <<"Client mknod"<< std::endl;
    std::cerr << "mode" << mode << "dev" << dev << std::endl;
#endif
    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"mknod", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_mknod will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {//rpc_call failed
        fxn_ret = -EINVAL;
    } else {
        // might the rpcCall succeed but the return failed
# ifdef PRINT_ERR
        std::cerr <<"Client mknod"<< std::endl;
        std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;
    }

    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_open_remote(void *userdata, const char *path, struct fuse_file_info *fi) {
    // the system call returns the file descriptor or error
    // the ret_code denotes whether the system call is succeed or not

    // Called during open.
    // You should fill in fi->fh.

    // SET UP THE RPC CALL

    // mknod has 4 arguments.
    int num_args = 3;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The second argument
    arg_types[1] = (1 << ARG_OUTPUT) |(1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void*) fi;

    // The 4th argument
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"open", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {//rpcCall succeed
        if (ret_code >= 0){// the file is correctly opened

            fxn_ret = 0;
        }else{//
# ifdef PRINT_ERR
            std::cerr <<"Client Failed to open"<< std::endl;
            std::cerr << "error code:" << ret_code << std::endl;
#endif
            fxn_ret = ret_code;
        }
    }
    // Clean up the memory we have allocated.
    free(args);
    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_release_remote(void *userdata, const char *path,
                       struct fuse_file_info *fi) {
    // Called during close, but possibly asynchronously.

    int num_args = 3;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The second argument
    arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void*) fi;

    // The 3rd argument
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"release", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {
        if (ret_code >= 0){// the file is correctly released
            fxn_ret = 0;
        }else{
            fxn_ret = ret_code;
        }
    }

    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}

// READ AND WRITE DATA
int watdfs_cli_read_remote(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    // Read size amount of data at offset of file into buf.
    // SET UP THE RPC CALL

    // mknod has 4 arguments.
    int num_args = 6;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // length of names
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;


    // The 5th argument
    arg_types[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[4] = (void*) fi;

    // The 6th argument
    arg_types[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[5] = (void*) & ret_code;

    // Last argument
    arg_types[6] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = 0;
    // Remember that size may be greater then the maximum array size of the RPC
    int fxn_ret = 0;
    int size_dynamic = size;
    int actually_read = 0;
    bool eof_meet = false;
    char * buf_tmp = (char *) malloc(sizeof(char)*MAX_ARRAY_LEN);
    size_t size_current_read;

    while (size_dynamic > 0){
        if (size_dynamic > MAX_ARRAY_LEN){
            size_current_read = MAX_ARRAY_LEN;
        }else{
            size_current_read = size_dynamic;
        }

        // The second argument
        arg_types[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | size_current_read;
        args[1] = (void*) buf_tmp;

        // The 3rd argument
        arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;
        args[2] = (void*) &size_current_read;

        // The 4th argument
        arg_types[3] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;
        args[3] = (void*) &offset;

        rpc_ret = rpcCall((char *)"read", arg_types, args);

        if (rpc_ret < 0) {
            fxn_ret = -EINVAL;
            break;
        } else {
            if (ret_code < 0){
                break;
            }else{// read file successful
                memcpy(buf+actually_read, buf_tmp, ret_code);
                actually_read = actually_read + ret_code;
                fxn_ret = actually_read;
                if (ret_code < size_current_read){//EOF meet
                    break;
                }
            }
        }
        size_dynamic = size_dynamic - size_current_read;
        offset = offset + size_current_read;
    }

    free(buf_tmp);

    // Clean up the memory we have allocated.
    free(args);

    // return number of bytes actually read
    return fxn_ret;
}

int watdfs_cli_write_remote(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    // Write size amount of data at offset of file from buf.

    // Remember that size may be greater then the maximum array size of the RPC
    // library.
    // mknod has 4 arguments.
    int num_args = 6;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // length of names
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The 5th argument
    arg_types[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[4] = (void*) fi;

    // The 6th argument
    arg_types[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[5] = (void*) & ret_code;

    // Last argument
    arg_types[6] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = 0;
    int fxn_ret = 0;
    int size_dynamic = size;
    int actually_write = 0;
    size_t size_current_write;

    while (size_dynamic > 0){
        std::cerr << "size: " << size_dynamic << std::endl;
        if (size_dynamic > MAX_ARRAY_LEN){
            size_current_write = MAX_ARRAY_LEN;
        }else{
            size_current_write = size_dynamic;
        }

        // The second argument
        arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | size_current_write;
        args[1] = (void*) buf;

        // The 3rd argument
        arg_types[2] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;
        args[2] = (void*) &size_current_write;

        // The 4th argument
        arg_types[3] = (1 << ARG_INPUT) | (ARG_LONG << 16) ;
        args[3] = (void*) &offset;

        rpc_ret = rpcCall((char *)"write", arg_types, args);

        if (rpc_ret < 0) {
            fxn_ret = -EINVAL;
            break;
        } else {
            if (ret_code < 0){
                fxn_ret = ret_code;
                break;
            }else{
                actually_write = actually_write + ret_code;
                fxn_ret = actually_write;
                if (ret_code < size_current_write){//EOF meet
                    break;
                }
            }
        }
        size_dynamic = size_dynamic - size_current_write;
        offset = offset + size_current_write;
    }

    // Clean up the memory we have allocated.
    free(args);

    // return number of bytes actually read
    return fxn_ret;
}

int watdfs_cli_truncate_remote(void *userdata, const char *path, off_t newsize) {
    // Change the file size to newsize.

    int num_args = 3;

    // Allocate space for the output arguments.
    void **args = (void**) malloc( num_args*sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // The second argument
    arg_types[1] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    args[1] = (void*) &newsize;

    // The 3rd argument
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"truncate", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {
        fxn_ret = ret_code;
    }

    free(args);
    return fxn_ret;
}

int watdfs_cli_fsync_remote(void *userdata, const char *path,
                     struct fuse_file_info *fi) {
    // Force a flush of file data.

    int num_args = 3;
    void **args = (void**) malloc(num_args * sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // fi
    arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void*) fi;

    // return code
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    // The return code is not an array, so we need to hand args[3] an int*.
    int ret_code; //*************** return code on stack *******************
    args[2] = (void*) &ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"fsync", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {//rpcCall succeed
        fxn_ret = ret_code;
    }

    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CHANGE METADATA
int watdfs_cli_utimens_remote(void *userdata, const char *path,
                       const struct timespec ts[2]) {
    // Change file access and modification times.

    int num_args = 3;
    void **args = (void**) malloc(num_args * sizeof(void*));

    // Allocate the space for arg types, and one extra space for the null
    // array element.
    int arg_types[num_args + 1];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;

    // Fill in the arguments
    // The first argument is the path
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void*)path;

    // ts
    arg_types[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY)| (ARG_CHAR << 16) | 2*sizeof(struct timespec);
    args[1] = (void*) ts;

    // return code
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ;
    // The return code is not an array, so we need to hand args[3] an int*.
    int ret_code; //*************** return code on stack *******************
    args[2] = (void*) &ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"utimens", arg_types, args);

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } else {//rpcCall succeed
        fxn_ret = ret_code;
    }

    // Clean up the memory we have allocated.
    free(args);

    // Finally return the value we got from the server.
    return fxn_ret;
}
