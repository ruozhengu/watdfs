//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"
#include "debug.h"
INIT_LOG
#include <math.h>

#include "rpc.h"
#include <iostream>
/*
You should implement functions in the following order: getattr, mknod, open, release, and fgetattr.
These will allow you to support opening and creating a file. Next, you should implement: write, read, and truncate.
These will allow you to read and write data to a file. Finally implement: utimens and fsync.
*/

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *ret_code) {
    // TODO: set up the RPC library by calling `rpcClientInit`.
    int initRet = rpcClientInit();

    if (initRet == 0) DLOG("Client Init Success");
    else DLOG("Client Init Fail");

    *ret_code = initRet;

    // TODO: check the return code of the `rpcClientInit` call
    // `rpcClientInit` may fail, for example, if an incorrect port was exported.

    // It may be useful to print to stderr or stdout during debugging.
    // Important: Make sure you turn off logging prior to submission!
    // One useful technique is to use pre-processor flags like:
    // # ifdef PRINT_ERR
    // std::cerr << "Failed to initialize RPC Client" << std::endl;
    // #endif
    // Tip: Try using a macro for the above to minimize the debugging code.
    // TODO Initialize any global state that you require for the assignment and return it.
    // The value that you return here will be passed as userdata in other functions.
    // In A1, you might not need it, so you can return `nullptr`.
    void *userdata = nullptr;

    // TODO: save `path_to_cache` and `cache_interval` (for A3).

    // TODO: set `ret_code` to 0 if everything above succeeded else some appropriate
    // non-zero value.

    // Return pointer to global state data.
    return userdata;
}

void watdfs_cli_destroy(void *userdata) {

    int destoryRet = rpcClientDestroy();

    if (!destoryRet) DLOG("Client Destory Success");
    else DLOG("Client Destory Fail");

    userdata = nullptr;
}

// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL
    DLOG("Received getattr call from local client...");

    // getattr has 3 arguments.
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

    // The second argument is the stat structure. This argument is an output
    // only argument, and we treat it as a char array. The length of the array
    // is the size of the stat structure, which we can determine with sizeof.
    arg_types[1] = (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
                   (uint)sizeof(struct stat); // statbuf
    args[1] = (void *)statbuf;

    // The third argument is the return code, an output only argument, which is
    // an integer.
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u); // return code

    // The return code is not an array, so we need to hand args[2] an int*.
    // The int* could be the address of an integer located on the stack, or use
    // a heap allocated integer, in which case it should be freed.
    int ret_code = 0;
    args[2] = (void *)&ret_code;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"getattr", arg_types, args);
    DLOG("DONE: getattr: code is %d", rpc_ret);
    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        DLOG( "rpcCall on getattr is failing");
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) {
        // Important: if the return code of watdfs_cli_getattr is negative (an
        // error), then we need to make sure that the stat structure is filled
        // with 0s. Otherwise, FUSE will be confused by the contradicting return
        // values.
        memset(statbuf, 0, sizeof(struct stat));
    }

    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: getattr: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {
    // SET UP THE RPC CALL
    DLOG("Received fgetattr call from local client...");

    // getattr has 3 arguments.
    int ARG_COUNT = 4;

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

    // The second argument is the stat structure. This argument is an output
    // only argument, and we treat it as a char array. The length of the array
    // is the size of the stat structure, which we can determine with sizeof.
    arg_types[1] = (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
                   (uint)sizeof(struct stat); // statbuf
    args[1] = (void *)statbuf;

    // The third argument is the fi structure. This argument is an input
    // only argument, and we treat it as a char array.
    arg_types[2] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
        (uint)sizeof(struct fuse_file_info);
    args[2] = (void *)fi;

    // The fourth argument is the return code, an output only argument, which is
    // an integer.
    arg_types[3] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

    // The return code is not an array, so we need to hand args[2] an int*.
    // The int* could be the address of an integer located on the stack, or use
    // a heap allocated integer, in which case it should be freed.
    int ret_code = 0;
    args[3] = (void *)&ret_code;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[4] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"fgetattr", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        DLOG( "rpcCall on fgetattr is failing");
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) {
        // Important: if the return code of watdfs_cli_getattr is negative (an
        // error), then we need to make sure that the stat structure is filled
        // with 0s. Otherwise, FUSE will be confused by the contradicting return
        // values.
        // memset(statbuf, 0, sizeof(struct stat));
        DLOG("ggetattr get negative return code, read might fail ...");
    }

    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: fgetattr: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    // Called to create a file.
    // SET UP THE RPC CALL
    DLOG("Received mknod rpcCall from local client...");

    // getattr has 4 arguments.
    int ARG_COUNT = 4;

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



    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: mknod: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;

}
int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {
    // Called to open a file.
    // SET UP THE RPC CALL
    DLOG("Received open rpcCall from local client...");

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


    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: open: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path,
                       struct fuse_file_info *fi) {
    // Called to release a file.
    // SET UP THE RPC CALL
    DLOG("Received release rpcCall from local client...");

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
    arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;

    // The second argument is the fi structure. This argument is an input
    // only argument, and we treat it as char array
    arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
        (uint)sizeof(struct fuse_file_info);


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
    int rpc_ret = rpcCall((char *)"release", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        DLOG( "Release rpcCall: fail");
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0) DLOG("Release rpcCall: return code is negative");


    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: open: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    /*
      write buf to the file at offset location with size amount.
      Sicne array has limited size, we need to make multiple rpcCall to perform write
    */

    DLOG("Received read rpcCall from local client...");

    // MAKE THE RPC CALL
    size_t readRemain = size, rpcSize = MAX_ARRAY_LEN;
    int ret_code = 0, fxn_ret = 0, total = 0;
    off_t next = offset;

    while (readRemain > rpcSize) {

      // getattr has 7 arguments.
      int ARG_COUNT = 6;

      //initialization
      void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
      int arg_types[ARG_COUNT + 1];
      int pathlen = strlen(path) + 1;

      // Fill in the arguments

      // path is an input only argument, and a char array.
      arg_types[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;

      //size is input only argument and long type.
      arg_types[2] =  (1u << ARG_INPUT) | (ARG_LONG << 16u);

      // offset is input only argument and long type .
      arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

      // fi is an input only argument, and a char array.
      arg_types[4] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
        (uint)sizeof(struct fuse_file_info);

      // return code is output only argument and is integer
      arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

      // set last position to 0
      arg_types[6] = 0;

      //second arg is buffer, which is input only and char array
      arg_types[1] =
        (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)rpcSize;

      // update actual args
      args[0] = (void *)path;
      args[1] = (void *)buf;
      args[2] = (void *)&rpcSize;
      args[3] = (void *)&next;
      args[4] = (void *)fi;
      args[5] = (void *)&ret_code;

      // calling rpc
      int rpc_ret = rpcCall((char *)"read", arg_types, args);

      if (rpc_ret < 0) {
        DLOG("CURRENT READ RPC CALL FAIL");
        fxn_ret = -EINVAL;
        return fxn_ret;
      }

      if (ret_code < 0) {
        DLOG("CURRENT READ SERVER FAIL");
        fxn_ret = ret_code;
        return fxn_ret;
      }

      if (ret_code < MAX_ARRAY_LEN){
        DLOG("CURRENT READ FINISH + SUCCEEDDDD");
        fxn_ret = total + ret_code;
        return fxn_ret;
      }

      next += ret_code;
      total += ret_code;
      buf = buf + ret_code; //ptr arithmetic
      readRemain -= rpcSize;


      // Clean up the memory we have allocated.
      free(args);
    } // END OF LOOP

    rpcSize = readRemain;

    // getattr has 7 arguments.
    int ARG_COUNT = 6;

    //initialization
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int arg_types[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;

    // Fill in the arguments

    // path is an input only argument, and a char array.
    arg_types[0] =
      (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;

    //size is input only argument and long type.
    arg_types[2] =  (1u << ARG_INPUT) | (ARG_LONG << 16u);

    // offset is input only argument and long type .
    arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

    // fi is an input only argument, and a char array.
    arg_types[4] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
      (uint)sizeof(struct fuse_file_info);

    // return code is output only argument and is integer
    arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);

    // set last position to 0
    arg_types[6] = 0;

    //second arg is buffer, which is input only and char array
    arg_types[1] =
      (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)rpcSize;

    // update actual args
    args[0] = (void *)path;
    args[1] = (void *)buf;
    args[2] = (void *)&rpcSize;
    args[3] = (void *)&next;
    args[4] = (void *)fi;
    args[5] = (void *)&ret_code;

    int rpc_ret = rpcCall((char *)"read", arg_types, args);

    if(rpc_ret < 0){
        DLOG( "READ: LAST RPC CALL FAIL");
        fxn_ret = -EINVAL;
    } else if(ret_code < 0){
        DLOG( "READ: LAST SERVER FAIL");
        fxn_ret = ret_code;
    } else {
        fxn_ret = total + ret_code;
    }

    free(args);


    if (fxn_ret < 0) {
      DLOG("Read rpcCall: return code is negative");
      return fxn_ret;
    }

    DLOG("DONE: read: return code is %d", fxn_ret);

    // Return the requested bytes to read marks a success
    return fxn_ret;
}

// READ AND WRITE DATA
int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    // Write size amount of data at offset of file from buf.

    // Remember that size may be greater then the maximum array size of the RPC
    // library.
    // Read size amount of data at offset of file into buf.
    DLOG("write is called from client...");
    int ARG_COUNT = 6;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int arg_types[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;

    arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint) pathlen;
    arg_types[2] = (1u << ARG_INPUT) | (ARG_LONG << 16u);
    arg_types[3] = (1u << ARG_INPUT) | (ARG_LONG << 16u) ;
    arg_types[4] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint) sizeof(struct fuse_file_info) ;
    arg_types[5] = (1u << ARG_OUTPUT) | (ARG_INT << 16u) ;
    arg_types[6] = 0;

    int rpc_ret, total_ret = 0, fxn_ret = 0;
    size_t size_remain = size;
    args[0] = (void *)path;
    args[4] = (void *)fi;


    while(size_remain > MAX_ARRAY_LEN){
        size = MAX_ARRAY_LEN;
        args[1] = (void *)buf;
        args[2] = (void *) &size;
        args[3] = (void *) &offset;
        int write_ret;
        args[5] = (void *) &write_ret;
        arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint) size;
        rpc_ret = rpcCall((char *)"write", arg_types, args);

        if(rpc_ret < 0){
            DLOG( "rpcCall_read fail");  // rpc call fail
            fxn_ret = -EINVAL;
            return fxn_ret;
        }
        else if(write_ret < 0){        // server fail
            DLOG("Imply read on server fail");
            fxn_ret = write_ret;
            return fxn_ret;
        }
        else if(write_ret < MAX_ARRAY_LEN){
            DLOG("finish read on client"); // finish write because can't write more from buf
            fxn_ret = total_ret + write_ret;
            return fxn_ret;
        }
        else{
            buf += write_ret; // update buf
            offset += write_ret; // update offset
            total_ret += write_ret; // update total read bytes
            size_remain -= MAX_ARRAY_LEN; // remain size need to read
        }
    }

    size = size_remain;
    args[1] = (void *) buf;
    args[2] = (void *) &size;
    args[3] = (void *) &offset;
    int write_ret;
    args[5] = (void *) &write_ret;
    arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint) size;
    rpc_ret = rpcCall((char *)"write", arg_types, args);

    if (rpc_ret < 0) {
        DLOG( "rpcCall_write fail");
        fxn_ret = -EINVAL;
    } else {
        fxn_ret = write_ret;
    }

    if (fxn_ret < 0) {
        DLOG("Imply write on server fail");
    }else{
        fxn_ret = total_ret + write_ret; // update total read bytes
    }

    free(args);
    DLOG("Return code from server : %d", fxn_ret);
    return fxn_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {
  // Called to release a file.
  // SET UP THE RPC CALL
  DLOG("Received truncate rpcCall from local client...");

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
  arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
  // For arrays the argument is the array pointer, not a pointer to a pointer.
  args[0] = (void *)path;

  // The second argument is the newsize var. This argument is an input
  // only argument, and we treat it as integer
  arg_types[1] = (1u << ARG_INPUT) | (ARG_LONG << 16u);

  args[1] = (void *)&newsize;

  // The second argument is return code, an output only argument, which is
  // an integer type.
  arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
  int ret_code = 0;
  args[2] = (void *)&ret_code;

  // Finally, the last position of the arg types is 0. There is no
  // corresponding arg.
  arg_types[3] = 0;

  // MAKE THE RPC CALL
  int rpc_ret = rpcCall((char *)"truncate", arg_types, args);

  // HANDLE THE RETURN
  int fxn_ret = 0;
  if (rpc_ret < 0) {
      // Something went wrong with the rpcCall, return a sensible return
      // value. In this case lets return, -EINVAL
      DLOG( "Truncate rpcCall: fail");
      fxn_ret = -EINVAL;
  } else {
      // Our RPC call succeeded. However, it's possible that the return code
      // from the server is not 0, that is it may be -errno. Therefore, we
      // should set our function return value to the retcode from the server.
      fxn_ret = ret_code;
  }

  if (fxn_ret < 0) DLOG("Truncate rpcCall: return code is negative");


  // Clean up the memory we have allocated.
  free(args);

  DLOG("DONE: truncate: return code is %d", fxn_ret);

  // Finally return the value we got from the server.
  return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {
  // Called to release a file.
  // SET UP THE RPC CALL
  DLOG("Received fsync rpcCall from local client...");

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
  arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
  args[0] = (void *)path;

  // The second argument is the fi. This argument is an input
  // only argument, and we treat it as char array
  arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
   (uint)sizeof(struct fuse_file_info);
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
  int rpc_ret = rpcCall((char *)"fsync", arg_types, args);

  // HANDLE THE RETURN
  int fxn_ret = 0;
  if (rpc_ret < 0) {
    // Something went wrong with the rpcCall, return a sensible return
    // value. In this case lets return, -EINVAL
    DLOG( "Fsync rpcCall: fail");
    fxn_ret = -EINVAL;
  } else {
    // Our RPC call succeeded. However, it's possible that the return code
    // from the server is not 0, that is it may be -errno. Therefore, we
    // should set our function return value to the retcode from the server.
    fxn_ret = ret_code;
  }

  if (fxn_ret < 0) DLOG("Fsync rpcCall: return code is negative");


  // Clean up the memory we have allocated.
  free(args);

  DLOG("DONE: truncate: return code is %d", fxn_ret);

  // Finally return the value we got from the server.
  return fxn_ret;
}

// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path,
                       const struct timespec ts[2]) {

    // Called to release a file.
    // SET UP THE RPC CALL
    DLOG("Received utimens rpcCall from local client...");

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
    arg_types[0] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)pathlen;
    args[0] = (void *)path;

    // The second argument is the ts. This argument is an input
    // only argument, and we treat it as char array
    arg_types[1] = (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) |
      (uint)sizeof(struct timespec) * 2;
    args[1] = (void *)ts;

    // The second argument is return code, an output only argument, which is
    // an integer type.
    arg_types[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    int ret_code = 0;
    args[2] = (void *)&ret_code;

    // Finally, the last position of the arg types is 0. There is no
    // corresponding arg.
    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"utimens", arg_types, args);

    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
       // Something went wrong with the rpcCall, return a sensible return
       // value. In this case lets return, -EINVAL
       DLOG( "Utimens rpcCall: fail");
       fxn_ret = -EINVAL;
    } else {
       // Our RPC call succeeded. However, it's possible that the return code
       // from the server is not 0, that is it may be -errno. Therefore, we
       // should set our function return value to the retcode from the server.
       fxn_ret = ret_code;
    }

    if (fxn_ret < 0) DLOG("Utimens rpcCall: return code is negative");


    // Clean up the memory we have allocated.
    free(args);

    DLOG("DONE: truncate: return code is %d", fxn_ret);

    // Finally return the value we got from the server.
    return fxn_ret;
}
