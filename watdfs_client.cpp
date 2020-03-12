//
// Starter code for CS 454/654
// You SHOULD change this file
//
#include "watdfs_client.h"
#include "debug.h"
INIT_LOG

#include "rpc.h"
#include <string>
#include <cstdint>
#include <sys/stat.h>
#include <map>
#include <iostream>
#include <cstring>


#include "rpc.h"
int readonly = O_RDONLY;


// ------------ define util global var below ----------------------

// // global chache states to be init
time_t cacheInterval;
char *cachePath;

// track descriptor and last access time of each file
struct fileMetadata {
  int client_mode;
  int server_mode;
  time_t tc;
};

// track opened files by clients, just a type; key is not full path!
typedef std::map<std::string, struct fileMetadata *> openFiles;

// ------------------ COPY OLD RPC CALLS --------------------------------

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
      //DLOG(.*)
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
  std::string p = std::string(path);
  return open_files->count(p) > 0 ? true : false;
}

struct fileMetadata * get_file_metadata(openFiles *open_files, const char *path) {
  return (*open_files)[std::string(path)];
}

// to be called in download function
// to write from server to local
static int _write(void *userdata, const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
  userdata = (void *) userdata;
  int sys_ret = pwrite(fi->fh, buf, size, offset);

  // handle error
  if (sys_ret < 0) {
    //DLOG(.*)
    sys_ret = -errno;
  }
  return sys_ret;
}

// // to be called in upload function
// // to read from local to server
// static int _read(void *userdata, const char *path, const char *buf, size_t size,
//                     off_t offset, struct fuse_file_info *fi) {
//
//     int sys_ret = pread(fi->fh, buf, size, offset);
//
//     // handle error
//     if (sys_ret < 0) {
//       //DLOG(.*)
//       sys_ret = -errno;
//     }
//     return sys_ret;
// }

int write_from_buffer_to_local_file(void *userdata, const char *path, const char *buf,
                              size_t size, off_t offset){
    // Write size amount of data at offset of file from buf.

    // MAKE THE RPC CALL
    int sys_ret = 0;
    int fxn_ret = 0;
    int size_dynamic = size;
    int actually_write = 0;
    size_t size_current_write;

    //std::cerr <<"write_from_buffer_to_local_file fh  "<< ((global_state*)userdata)->opened_files[path]->fh_client_local << std::endl;

    while (size_dynamic > 0){
        if (size_dynamic > MAX_ARRAY_LEN){
            size_current_write = MAX_ARRAY_LEN;
        }else{
            size_current_write = size_dynamic;
        }

        sys_ret = pwrite((*((openFiles*)userdata))[path]->client_mode, buf, size_current_write, offset);

        if (sys_ret< 0){
            fxn_ret = -errno;
            break;
        }else{
            actually_write = actually_write + sys_ret;
            fxn_ret = size;
            if (sys_ret < size_current_write){//EOF meet
                break;
            }
        }

        size_dynamic = size_dynamic - size_current_write;
        offset = offset + size_current_write;
    }


    return fxn_ret;
}


static int download_to_client(void *userdata, const char *path, struct fuse_file_info *fi){

    // //DLOG(.*)
    //
    // int sys_ret = lock(path, RW_READ_LOCK);
    //
    // if (sys_ret < 0){
    //
    //   //DLOG(.*)
    //   return sys_ret;
    //
    // } else {
    //
    //   //DLOG(.*)
    //
    //   int fxn_ret = sys_ret;
    //
    //   struct stat *statbuf = new struct stat;
    //
    //   int rpc_ret = rpcCall_getattr(userdata, path, statbuf);
    //
    //   if (rpc_ret < 0){
    //       unlock(path, RW_READ_LOCK);
    //       delete statbuf;
    //       return rpc_ret;
    //   }
    //
    //   //DLOG(.*)
    //
    //   char *cache_path = get_cache_path(path);
    //
    //   size_t size = statbuf->st_size;
    //
    //   // open local file
    //   sys_ret = open(cache_path, O_CREAT | O_RDWR, S_IRWXU);
    //
    //   if (sys_ret < 0) {
    //     //DLOG(.*)
    //     free(cache_path);
    //     delete statbuf;
    //     unlock(path, RW_READ_LOCK);
    //     return -errno;
    //   }
    //
    //   int local_fh_retcode = sys_ret;
    //   struct fileMetadata *target = (*((openFiles *) userdata))[path];
    //   target->client_mode = local_fh_retcode;
    //
    //   // step1: truncate local file
    //   sys_ret = truncate(cache_path, (off_t)size);
    //
    //   if (sys_ret < 0){
    //       free(cache_path);
    //       delete statbuf;
    //       unlock(path, RW_READ_LOCK);
    //       return -errno;
    //   }
    //
    //   //DLOG(.*)
    //
    //   // read the file from server
    //   char *buf = (char *) malloc(((off_t) size) * sizeof(char));
    //
    //   rpc_ret = rpcCall_read(userdata, path, buf, size, 0, fi);
    //
    //   if (rpc_ret < 0){
    //       free(buf);
    //       delete statbuf;
    //       free(cache_path);
    //       unlock(path, RW_READ_LOCK);
    //       return rpc_ret;
    //   }
    //
    //   //DLOG(.*)
    //
    //   // write from server to local
    //   sys_ret = _write(userdata, path, buf, size, 0, fi); //TODO: not sure
    //
    //   if (sys_ret < 0){
    //     unlock(path, RW_READ_LOCK);
    //     free(buf);
    //     delete statbuf;
    //     free(cache_path);
    //     return sys_ret;
    //   }
    //
    //   //DLOG(.*)
    //
    //   // update metadata
    //   target->client_mode = local_fh_retcode;// server
    //   target->server_mode = fi->fh;// local
    //
    //   // update Tclient = Tserver and Tc = current time
    //   struct timespec t[2];
    //
    //   t[0] = (struct timespec)(statbuf->st_mtim);
    //   t[1] = (struct timespec)(statbuf->st_mtim);
    //
    //   int dirfd = 0;
    //   int flag = 0;
    //   // update the timestamps of a file by calling utimensat
    //   sys_ret = utimensat(dirfd, cache_path, t, flag);
    //
    //   //DLOG(.*)
    //
    //
    //   sys_ret = unlock(path, RW_READ_LOCK);
    //
    //   if (sys_ret < 0){
    //       free(buf);
    //       free(cache_path);
    //       // delete ts;
    //       delete statbuf;
    //       return sys_ret;
    //   }
    //
    //   //DLOG(.*)
    //
    //
    //   free(buf);
    //   free(cache_path);
    //
    //   delete statbuf;
    //   fxn_ret = sys_ret;
    //   return fxn_ret;
    // }

    int dfs_result, sys_ret, fxn_ret;
    char * full_path = get_cache_path(path);

    // Get file attributes from the server
    struct stat* statbuf = new struct stat;

    ////std::cerr << "remote getattr called in open" << std::endl;
    dfs_result = rpcCall_getattr(userdata, path, statbuf);
    //std::cerr << "remote getattr finished in open" << dfs_result << std::endl;
    if (dfs_result < 0){
        ////std::cerr << "remote getattr failed in open" << std::endl;
        fxn_ret = dfs_result;
        delete statbuf;
        free(full_path);
        return fxn_ret;
    }
    off_t size_of_file = statbuf->st_size;

    // open the file
    int local_fh;
    sys_ret = open(full_path, O_CREAT | O_RDWR, 00777);// open locally
    //std::cerr << "local open finished in open" << sys_ret << " file size" << size_of_file << std::endl;

    if (sys_ret < 0) {
        //std::cerr << "local open failed " << errno << std::endl;
        ////std::cerr << "local open failed on client faield" << std::endl;
        // If there is an error on the system call, then the return code should
        // be -errno.
        fxn_ret = -errno;
        delete statbuf;
        free(full_path);
        return fxn_ret;
    }else{
        local_fh = sys_ret;
        (*((openFiles *) userdata))[path]->client_mode = local_fh;
    }

    // truncate the file at the client
    sys_ret = truncate(full_path, size_of_file);
    //std::cerr << "truncate finished in open" << sys_ret << " file size " << size_of_file << std::endl;
    if (sys_ret < 0){
        //std::cerr << "error " << errno << std::endl;
        fxn_ret = -errno;
        delete statbuf;
        free(full_path);
        return fxn_ret;
    }

    // read the file from the server, stored in the buffer
    char * buffer = (char *) malloc( size_of_file*sizeof(char));
    //std::cerr << "read remote on client" << std::endl;

    std::cerr << "download lock acquired" << std::endl;
    sys_ret = lock(path, RW_READ_LOCK);
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }
    dfs_result = rpcCall_read(userdata, path, buffer, size_of_file, 0, fi);
    if (dfs_result < 0){
        fxn_ret = dfs_result;
        delete statbuf;
        free(buffer);
        free(full_path);
        return fxn_ret;
    }
    sys_ret = unlock(path, RW_READ_LOCK);
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }
    std::cerr << "download lock released" << std::endl;

    // write the file to the client
    // sys_ret is the fh
    //std::cerr << "write from buffer to file" << std::endl;
    sys_ret = write_from_buffer_to_local_file(userdata, path, buffer, size_of_file, 0);
    ////std::cerr << "write from buffer to file finished in open" << sys_ret << std::endl;
    if (sys_ret < 0){
        fxn_ret = sys_ret;
        delete statbuf;
        free(buffer);
        free(full_path);
        return fxn_ret;
    }

    // update the file metadata at the client
    (*((openFiles *) userdata))[path]->server_mode = fi->fh;// server
    (*((openFiles *) userdata))[path]->client_mode = local_fh;// local

    // ******** flags_local is setted in open, as the mode setted by the FUSE application, the original one

    //local call utimens, set the modified time
    struct timespec * ts = new struct timespec [2];
    ts[1] = statbuf->st_mtim;
    ts[0] = statbuf->st_atim;

    sys_ret = utimensat(0, full_path, ts, 0);

    //std::cerr <<"file opened " << file_opened(userdata, path) << " local fh " << local_fh << std::endl;

    delete statbuf;
    free(buffer);
    free(full_path);
    fxn_ret = sys_ret;

    // Finally return the value we got from the server.
    return fxn_ret;
}


static int push_to_server(void *userdata, const char *path, struct fuse_file_info *fi){

    //DLOG(.*)

    int sys_ret = lock(path, RW_WRITE_LOCK);

    if (sys_ret < 0){

      //DLOG(.*)
      return sys_ret;

    } else {

      //DLOG(.*)

      int fxn_ret = sys_ret;

      struct stat *statbuf = new struct stat;
      char *cache_path = get_cache_path(path);
      sys_ret = stat(cache_path, statbuf);

      if (sys_ret < 0) {
          memset(statbuf, 0, sizeof(struct stat));
          delete statbuf;
          return sys_ret;
      }
      fxn_ret = sys_ret;

      size_t size = statbuf->st_size;
      char *buf = (char *) malloc(((off_t) size) * sizeof(char));

      // step1: truncate local file
      sys_ret = rpcCall_truncate(userdata, path, (off_t) size);

      if (sys_ret < 0){
          free(cache_path);
          free(buf);
          delete statbuf;
          unlock(path, RW_WRITE_LOCK);
          return sys_ret;
      }

      //DLOG(.*)

      // read the file from client


      int rpc_ret = pread(fi->fh, buf, size, 0);


      if (rpc_ret < 0){
          free(buf);
          delete statbuf;
          free(cache_path);
          unlock(path, RW_WRITE_LOCK);
          return rpc_ret;
      }

      //DLOG(.*)

      // write from server to local
      sys_ret = rpcCall_write(userdata, path, buf, size, 0, fi);

      if (sys_ret < 0){
        unlock(path, RW_WRITE_LOCK);
        free(buf);
        delete statbuf;
        free(cache_path);
        return sys_ret;
      }

      fxn_ret = sys_ret;

      //DLOG(.*)


      // update Tclient = Tserver and Tc = current time
      struct timespec t[2];

      t[0] = (struct timespec)(statbuf->st_mtim);
      t[1] = (struct timespec)(statbuf->st_mtim);


      // update the timestamps of a file by calling utimensat
      sys_ret = rpcCall_utimens(userdata, path, t);

      fxn_ret = sys_ret;

      if (sys_ret < 0) {
        free(cache_path);
        free(buf);
        delete statbuf;
        unlock(path, RW_WRITE_LOCK);
        return fxn_ret;
      }

      //DLOG(.*)

      sys_ret = unlock(path, RW_WRITE_LOCK);

      if (sys_ret < 0){
          free(buf);
          free(cache_path);
          delete statbuf;
          // delete ts;
          return sys_ret;
      }
      //DLOG(.*)


      free(buf);
      delete statbuf;
      free(cache_path);

      fxn_ret = sys_ret;
      return fxn_ret;
    }
}
double timespec_diff2(time_t T1, time_t T2){
    return (double) (difftime(T1.tv_sec, T2.tv_sec));
}

// w =1, r = 0
bool freshness_check(openFiles *open_files, const char *cache_path, const char *path, int rw_flag) {

  //DLOG(.*)

  int sys_ret, fxn_ret, dfs_ret;
    struct fileMetadata * file_meta = (*open_files)[path];
    time_t Tc = (file_meta->tc);
    time_t T = time(0));

    // get T_client and T_server
    struct stat* statbuf = new struct stat;
    char * full_path = get_cache_path(path);

    sys_ret = stat(full_path, statbuf);
    if (sys_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }
    struct timespec T_client = statbuf->st_mtim;

    dfs_ret = rpcCall_getattr((void *)open_files, path, statbuf);
    if (dfs_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        dfs_ret = sys_ret;
    }
    struct timespec T_server = statbuf->st_mtim;

    if (!( timespec_diff2(T, Tc ) < cacheInterval
        ||timespec_diff2(T_client, T_server) == 0)){
        //std::cerr << "file timeout and freshed" << std::endl;
        struct fuse_file_info * fi = new struct fuse_file_info;
        fi->fh = file_meta->server_mode;
        fi->flags = O_RDONLY;
        dfs_ret = download_to_client((void *)open_files, path, fi);

        if (dfs_ret < 0){
            delete fi;
            delete statbuf;
            return fxn_ret;
        }
    }

    // free
    delete statbuf;
    return fxn_ret;
}
// ------------------ END OF UTIL ----------------------------

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *ret_code) {
    // set up the RPC library by calling `rpcClientInit`.
    int initRet = rpcClientInit();

    if (initRet == 0) DLOG("Client Init Success");
    else DLOG("Client Init Fail");

    *ret_code = initRet;

    // init global trackers
    openFiles *userdata=  new std::map<std::string, fileMetadata*>(); //TODO: free it
    cacheInterval = cache_interval;
    cachePath = (char *)malloc(strlen(path_to_cache) + 1);
    strcpy(cachePath, path_to_cache);

    // Return pointer to global state data.
    return (void *) userdata;
}

//TODO: might need to free path as well
void watdfs_cli_destroy(void *userdata) {


    int destoryRet = rpcClientDestroy();

    if (!destoryRet) DLOG("Client Destory Success");
    else DLOG("Client Destory Fail");

    for(auto it : (*(openFiles *)userdata)) {
      delete it.second;
    }

    free(cachePath);
    // delete userdata;
    userdata = NULL;
}
bool check_if_file_exist_on_server(void *userdata, const char *path){
    int remote_ret = 0;
    struct stat * tmp_statbuf = new struct stat;
    remote_ret = rpcCall_getattr(userdata, path, tmp_statbuf);
    return (remote_ret != -2);
}
void set_validate_time(void * userdata, const char *path){
    (*((openFiles *) userdata))[path]->tc = time(0);
}
// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf){
    int flag = O_RDONLY;

    std::cerr <<"cli getattr called"<< std::endl;
    int fxn_ret = 0;
    if (!is_file_open((openFiles *)userdata, path)){// file not opened at the client side, should be closed
        std::cerr <<"file has not been opened"<< std::endl;

        /******* check if a file exist on the server *******/
        if (!check_if_file_exist_on_server(userdata, path)){// true if file does not exist
            std::cerr << "in getattr file does not exist on server" << std::endl;
            return -2;
        }
        /******* check if a file exist on the server *******/

        struct fuse_file_info* file_info = new struct fuse_file_info;
        file_info->flags = flag;

        int dfs_ret = watdfs_cli_open(userdata, path, file_info);
        if (dfs_ret < 0){
            delete file_info;
            return dfs_ret;
        }

        char *full_path = get_cache_path(path);
        // MAKE THE system call
        int sys_ret = stat(full_path, statbuf);

        if (sys_ret < 0) {
            fxn_ret = -errno;
            memset(statbuf, 0, sizeof(struct stat));
        }else{
            fxn_ret = sys_ret;
        }

        // Clean up the full path, it was allocated on the heap.
        free(full_path);
        //std::cerr <<"system return "<< sys_ret << std::endl;
        sys_ret = watdfs_cli_release(userdata, path, file_info);
        fxn_ret = sys_ret;
        std::cerr <<"client getattr finished "<< fxn_ret << std::endl;

        return fxn_ret;
    }else{// file already opened
        char *full_path = get_cache_path(path);

        int utils_ret = 0;
        utils_ret = freshness_check((openFiles *)userdata, full_path, path, 0);

        set_validate_time(userdata, path);
        if (utils_ret < 0){
            return utils_ret;
        }

        // MAKE THE system call
        int sys_ret = stat(full_path, statbuf);

        if (sys_ret < 0) {
            fxn_ret = -errno;
            memset(statbuf, 0, sizeof(struct stat));
        }else{
            fxn_ret = sys_ret;
        }

        std::cerr <<"file size "<< statbuf->st_size << std::endl;

        // Clean up the full path, it was allocated on the heap.
        free(full_path);
        //std::cerr <<"system return "<< sys_ret << std::endl;
        std::cerr <<"client getattr finished"<< std::endl;

        return fxn_ret;
    }
}

int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {

    DLOG("NEW: Received fgetattr call from local client...");

    int fxn_ret = 0;
    int ret_code = 0;
    int sys_ret = 0;


    char *cache_path = get_cache_path(path);

    if (is_file_open((openFiles *)userdata, path)) {

      DLOG("file opened before, checking freshness ...");

      // check client server consistency
      ret_code = freshness_check((openFiles *) userdata, cache_path, path, 0);

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
      struct stat * tmp = new struct stat;

      // return error code if file exists on server ...
      if (rpcCall_getattr(userdata, path, tmp) == -2) {

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
    free(cache_path);

    DLOG("DONE: mknod: return code is %d", fxn_ret);

    return fxn_ret;
}

int open_local_file(void *userdata, char *cache_path, int flags){
    std::string s_cache_path(cache_path);
    int ret;
    if(is_file_open((openFiles *)userdata, cache_path)){
        DLOG("file is open on client, can't open again");
        return -EMFILE;
    }
    ret = open(cache_path, flags);
    if(ret < 0){
        DLOG("open local file fail");
        return -errno;
    }else{
        // (*((openFiles *)userdata))[s_cache_path]->flags = flags;
        // (*((openFiles *)userdata))[s_cache_path]->fh = ret;
        (*((openFiles *)userdata))[s_cache_path]->tc = time(0);
        DLOG("open file and update metadata on client success");
        return 0;
    }
}

int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {
    struct stat *stat_server = (struct stat *)malloc(sizeof(struct stat));
    char *cache_path = get_cache_path(path);
    int ret, fxn_ret = 0;

    //check open
    if(is_file_open((openFiles *)userdata, cache_path)){
        fxn_ret = -EMFILE;
    }else{
        // check whether file exists on server
        ret = rpcCall_getattr(userdata, path, stat_server);
        if(ret < 0) {  // 文件在远端不存在
            DLOG("file doesn't exist on server");
            if(fi->flags != O_CREAT){     // 怎么查如果是O_CREAT | O_RD
                fxn_ret = ret;
                DLOG("don't allow to create a new file on server");
            }else{
                ret = rpcCall_open(userdata, path, fi);  // 远端既然不存在，用open新建不会报错
                if(ret < 0){
                    fxn_ret = ret;
                }
                ret = download_to_client(userdata, path, fi);  // 已经打开了远端server，不能再一次打开（写操作矛盾），顺便打开本地
                if(ret < 0){
                    fxn_ret = ret;
                }else{
                    DLOG("create a new file and download from server");
                }
            }
        }else {
            ret = download_to_client(userdata, path, fi);  // 打开本地文件
            if(ret < 0){
                fxn_ret = ret;
            }
        }
    }

    if(fxn_ret < 0){
        free(stat_server);
        free(cache_path);
        return fxn_ret;
    }else{
        ret = open_local_file(userdata, cache_path, fi->flags);
        if(ret < 0) {
            fxn_ret = ret;
        }
        free(stat_server);
        free(cache_path);
        return fxn_ret;
    }

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

    struct fileMetadata * target = (*((openFiles*)userdata))[std::string(path)];

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
    char *cache_path = get_cache_path(path);
    int ret_code = freshness_check((openFiles *)userdata, cache_path, path, 0);
    struct fileMetadata * target = (*((openFiles*)userdata))[std::string(path)];

    target->tc = time(NULL); // curr time

    if (ret_code < 0){
      fxn_ret = ret_code;
      DLOG("freshness check in read failed...");
      return fxn_ret;
    }

    //fxn_ret = _read(userdata, path, buf, size, offset, fi);
    fxn_ret = pread(fi->fh, buf, size, offset);

    // handle error
    if (fxn_ret < 0) {
      //DLOG(.*)
      fxn_ret = -errno;
    }


    if (fxn_ret < 0) return fxn_ret;
    return fxn_ret;
}


int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {

    DLOG("Received write rpcCall from local client...");
    int fxn_ret = 0;

    char *cache_path = get_cache_path(path);

    int sys_ret = _write(userdata, path, buf, size, offset, fi);
    if (sys_ret < 0) return sys_ret;


    int ret_code = freshness_check((openFiles *) userdata, cache_path, path, 1);
    struct fileMetadata * target = (*((openFiles*)userdata))[std::string(path)];

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
  fxn_ret = truncate(cache_path, newsize);

  // check freshness
  fxn_ret = freshness_check((openFiles *) userdata, cache_path, path, 1);

  if (fxn_ret < 0){
    DLOG("freshness check in truncate failed...");
    free(cache_path);
    return fxn_ret;
  }
  free(cache_path);

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
  struct fileMetadata * target = (*((openFiles*)userdata))[std::string(path)];

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
  int fxn_ret = stat(cache_path, statbuf);

  if (fxn_ret < 0) {
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
    ret_code = freshness_check((openFiles *)userdata, cache_path, path, 0);

    if (ret_code < 0){
      DLOG("utimens freshness check failed");
      return ret_code;
    }

    ret_code= utimensat(0, cache_path, ts, O_RDONLY);

  }

  if (is_mtime_diff){
    // need to check write freshness
    ret_code = freshness_check((openFiles *)userdata, cache_path, path, 1);

    if (ret_code < 0){
      DLOG("utimens freshness check failed");
      return ret_code;
    }

    ret_code = utimensat(0, cache_path, ts, O_WRONLY);

  }
  // extra sys call to utimensat
  if (is_mtime_diff) {
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
