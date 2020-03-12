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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// GET FILE ATTRIBUTES
int rpcCall_getattr(void *userdata, const char *path, struct stat *statbuf) {
    // SET UP THE RPC CALL


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
    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
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



    // Finally return the value we got from the server.
    return fxn_ret;
}

int rpcCall_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {
    // SET UP THE RPC CALL


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

    }

    // Clean up the memory we have allocated.
    free(args);

     

    // Finally return the value we got from the server.
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int rpcCall_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    // Called to create a file.
    // SET UP THE RPC CALL


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

        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0)



    // Clean up the memory we have allocated.
    free(args);



    // Finally return the value we got from the server.
    return fxn_ret;

}
int rpcCall_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {
    // Called to open a file.
    // SET UP THE RPC CALL


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

        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.


        fxn_ret = ret_code;
    }

    if (fxn_ret < 0)


    // Clean up the memory we have allocated.
    free(args);



    // Finally return the value we got from the server.
    return fxn_ret;
}

int rpcCall_release(void *userdata, const char *path,
                       struct fuse_file_info *fi) {
    // Called to release a file.
    // SET UP THE RPC CALL


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

        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.
        fxn_ret = ret_code;
    }

    if (fxn_ret < 0)


    // Clean up the memory we have allocated.
    free(args);



    // Finally return the value we got from the server.
    return fxn_ret;
}

// READ AND WRITE DATA
int rpcCall_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    /*
      write buf to the file at offset location with size amount.
      Sicne array has limited size, we need to make multiple rpcCall to perform write
    */



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

        fxn_ret = -EINVAL;
        return fxn_ret;
      }

      if (ret_code < 0) {

        fxn_ret = ret_code;
        return fxn_ret;
      }

      if (ret_code < MAX_ARRAY_LEN){

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

        fxn_ret = -EINVAL;
    } else if(ret_code < 0){

        fxn_ret = ret_code;
    } else {
        fxn_ret = total + ret_code;
    }

    free(args);


    if (fxn_ret < 0) {

      return fxn_ret;
    }



    // Return the requested bytes to read marks a success
    return fxn_ret;
}

// READ AND WRITE DATA
int rpcCall_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    /*
      write buf to the file at offset location with size amount.
      Sicne array has limited size, we need to make multiple rpcCall to perform write
    */



    // MAKE THE RPC CALL
    size_t writeRemain = size, rpcSize = MAX_ARRAY_LEN;
    int ret_code = 0, fxn_ret = 0, total = 0;
    off_t next = offset;

    while (writeRemain > rpcSize) {

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
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)rpcSize;

      // update actual args
      args[0] = (void *)path;
      args[1] = (void *)buf;
      args[2] = (void *)&rpcSize;
      args[3] = (void *)&next;
      args[4] = (void *)fi;
      args[5] = (void *)&ret_code;

      // calling rpc
      int rpc_ret = rpcCall((char *)"write", arg_types, args);

      if (rpc_ret < 0) {

        fxn_ret = -EINVAL;
        return fxn_ret;
      }

      if (ret_code < 0) {

        fxn_ret = ret_code;
        return fxn_ret;
      }

      if (ret_code < MAX_ARRAY_LEN){

        fxn_ret = total + ret_code;
        return fxn_ret;
      }

      next += ret_code;
      total += ret_code;
      buf = buf + ret_code; //ptr arithmetic
      writeRemain -= rpcSize;


      // Clean up the memory we have allocated.
      free(args);
    } // END OF LOOP

    rpcSize = writeRemain; //less than rpcsize

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
      (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | (uint)rpcSize;

    // update actual args
    args[0] = (void *)path;
    args[1] = (void *)buf;
    args[2] = (void *)&rpcSize;
    args[3] = (void *)&next;
    args[4] = (void *)fi;
    args[5] = (void *)&ret_code;

    int rpc_ret = rpcCall((char *)"write", arg_types, args);

    if(rpc_ret < 0){

        fxn_ret = -EINVAL;
    } else if(ret_code < 0){

        fxn_ret = ret_code;
    } else {
        fxn_ret = total + ret_code;
    }

    free(args);


    if (fxn_ret < 0) {

      return fxn_ret;
    }



    // Return the requested bytes to read marks a success
    return fxn_ret;
}

int rpcCall_truncate(void *userdata, const char *path, off_t newsize) {
  // Called to release a file.
  // SET UP THE RPC CALL


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

      fxn_ret = -EINVAL;
  } else {
      // Our RPC call succeeded. However, it's possible that the return code
      // from the server is not 0, that is it may be -errno. Therefore, we
      // should set our function return value to the retcode from the server.
      fxn_ret = ret_code;
  }

  if (fxn_ret < 0)


  // Clean up the memory we have allocated.
  free(args);



  // Finally return the value we got from the server.
  return fxn_ret;
}

int rpcCall_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {
  // Called to release a file.
  // SET UP THE RPC CALL


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

    fxn_ret = -EINVAL;
  } else {
    // Our RPC call succeeded. However, it's possible that the return code
    // from the server is not 0, that is it may be -errno. Therefore, we
    // should set our function return value to the retcode from the server.
    fxn_ret = ret_code;
  }

  if (fxn_ret < 0)


  // Clean up the memory we have allocated.
  free(args);



  // Finally return the value we got from the server.
  return fxn_ret;
}

// CHANGE METADATA
int rpcCall_utimens(void *userdata, const char *path,
                       const struct timespec ts[2]) {

    // Called to release a file.
    // SET UP THE RPC CALL


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

       fxn_ret = -EINVAL;
    } else {
       // Our RPC call succeeded. However, it's possible that the return code
       // from the server is not 0, that is it may be -errno. Therefore, we
       // should set our function return value to the retcode from the server.
       fxn_ret = ret_code;
    }

    if (fxn_ret < 0)


    // Clean up the memory we have allocated.
    free(args);



    // Finally return the value we got from the server.
    return fxn_ret;
}
