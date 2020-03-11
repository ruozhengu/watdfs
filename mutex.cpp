//
//  watdfs_mutex.cpp
//
//
//  Created by GenesisXun on 2019-03-16.
//

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "rpc.h"

int lock(const char *path, rw_lock_mode_t lock_mode) {
    // no if condition for whether
    // *********************** remote first ***********************
    std::cerr << "watdfs cli lock called" << std::endl;
    // if a file is on the server, there must be a copy in the local if we open it, the only situation
    int num_args = 3;

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
    args[1] = (void*) &lock_mode;

    // The 4th argument
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"lock", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_mknod will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {//rpc_call failed
        fxn_ret = -EINVAL;
    } else {
        // might the rpcCall succeed but the return failed
# ifdef PRINT_ERR
        //std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;

        if (ret_code < 0){
            free(args);
            return fxn_ret;
        }
    }

    // Finally return the value we got from the server.
    return fxn_ret;
}

int unlock(const char *path, rw_lock_mode_t lock_mode){
    // no if condition for whether
    // *********************** remote first ***********************
    std::cerr << "watdfs cli unlock called" << std::endl;
    // if a file is on the server, there must be a copy in the local if we open it, the only situation
    int num_args = 3;

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
    args[1] = (void*) &lock_mode;

    // The 4th argument
    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    // Finally, the last position of the arg types is 0. There is no corresponding
    // arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"unlock", arg_types, args);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_mknod will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {//rpc_call failed
        fxn_ret = -EINVAL;
    } else {
        // might the rpcCall succeed but the return failed
# ifdef PRINT_ERR
        //std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;

        if (ret_code < 0){
            free(args);
            return fxn_ret;
        }
    }

    // Finally return the value we got from the server.
    return fxn_ret;
}
