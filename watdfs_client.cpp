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
// time_t cacheInterval;
// char *cachePath;

// track descriptor and last access time of each file
struct fileMetadata {
  int client_mode;
  int server_mode;
  time_t tc;
};

// track opened files by clients, just a type; key is not full path!
struct file_state {
  time_t cacheInterval;
  char *cachePath;
  std::map<std::string, struct fileMetadata > openFiles;
};


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
bool is_file_open(struct file_state *userdata, const char *full_path){


    return (userdata->openFiles).find(std::string(full_path)) != (userdata->openFiles).end();
}

char *get_full_path(struct file_state *userdata, const char* rela_path) {
  int rela_path_len = strlen(rela_path);
  int dir_len = strlen(userdata->cachePath);

  int full_path_len = dir_len + rela_path_len + 1;

  char *full_path = (char *) malloc(full_path_len);

  strcpy(full_path, userdata->cachePath);
  strcat(full_path, rela_path);

  return full_path;
}

// bool is_file_open(struct file_state *open_files, const char *path) {
//   std::string p = std::string(path);
//   return (open_files->openFiles).count(p) > 0 ? true : false;
// }

struct fileMetadata get_file_metadata(struct file_state *userdata, const char *path) {
  return (userdata->openFiles)[std::string(path)];
}

// to be called in download function
// to write from server to local
static int _write(int ret, const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
  fi = (fuse_file_info *) fi;
  int sys_ret = pwrite(ret, buf, size, offset);

  // handle error
  if (sys_ret < 0) {
    //DLOG(.*)
    sys_ret = -errno;
  }
  return sys_ret;
}

void time_to_curr(void * userdata, const char * full_path) {
  time_t curr = time(0);
  (((file_state*)userdata)->openFiles)[std::string(full_path)].tc = curr;
}

static int download_to_client(struct file_state *userdata, const char *full_path,
                  const char *path){

    DLOG("download data from server");
    int flag1 = O_RDWR;
    int flag2 = O_CREAT;
    int flag3 = S_IRWXU;
    int flag4 = O_RDONLY;

    int fxn_ret;
    //lock it up first
    // int sys_ret = lock(path, RW_READ_LOCK);
    int sys_ret = 0;
    if (sys_ret < 0){

      DLOG("download lock acquire error");
      return sys_ret;

    } else {

      DLOG("download gets the lock");

      int fxn_ret = sys_ret;
      struct fuse_file_info *fi = new struct fuse_file_info;
      struct stat *statbuf = new struct stat;

      int rpc_ret = rpcCall_getattr((void *)userdata, path, statbuf);

      if (rpc_ret < 0){
          //unlock(path, RW_READ_LOCK);
          delete statbuf;
          delete fi;
          return rpc_ret;
      }


      size_t size = statbuf->st_size; // might need to cast to offset type

      // open local file
      sys_ret = open(full_path, flag1); //TODO???

      // loop until a new file is created
      if (sys_ret < 0) {
        DLOG("create a new file now");
        // trigger sys call for mknod
        mknod(full_path, statbuf->st_mode, statbuf->st_dev);

        sys_ret = open(full_path, flag1);
      }


      fi->flags = flag4;
      // (((userdata->openFiles)[full_path]).client_mode) = local_fh_retcode;

      // step1: truncate local file
      rpc_ret = truncate(full_path, (off_t)size);

      if (rpc_ret < 0) fxn_ret = -errno;


      //DLOG(.*)
      // TODO???? ret_code = rpc_open(userdata, path, fi);
      // read the file from server
      char *buf = (char *) malloc(((off_t) size) * sizeof(char));
      rpc_ret = rpcCall_open((void *)userdata, path, fi);
      if (rpc_ret < 0) fxn_ret = rpc_ret;

      rpc_ret = rpcCall_read((void *)userdata, path, buf, size, 0, fi);
      if (rpc_ret < 0) fxn_ret = rpc_ret;


      //DLOG(.*)

      // write from server to local
      rpc_ret = _write(sys_ret, path, buf, size, 0, fi); //TODO: not sure

      if (rpc_ret < 0){
        DLOG("download error");
        //unlock(path, RW_READ_LOCK);
        free(buf);
        delete statbuf;
        return sys_ret;
      }

      //DLOG(.*)

      // update metadata
      // target->client_mode = local_fh_retcode;// server
      // target->server_mode = fi->fh;// local

      // update Tclient = Tserver and Tc = current time
      struct timespec ts[2];

      ts[0] = (struct timespec)(statbuf->st_mtim);
      ts[1] = (struct timespec)(statbuf->st_mtim);

      int dirfd = 0;
      int flag = 0;
      // update the timestamps of a file by calling utimensat
      sys_ret = utimensat(dirfd, full_path, ts, flag);

      //TODO??? STAT?
      DLOG("download: metadata updated");
      rpc_ret = rpcCall_release((void *)userdata, path, fi);
      if (rpc_ret < 0) fxn_ret = rpc_ret;

      // close file locally
      int ret_code = close(sys_ret);
      if(ret_code < 0) fxn_ret = -errno;


      //rpc_ret = unlock(path, RW_READ_LOCK);

      // if (rpc_ret < 0){
      //   DLOG("download error");
      //   free(buf);
      //   // delete ts;
      //   delete statbuf;
      //   return sys_ret;
      // }

      DLOG("download succeed");


      free(buf);
      delete fi;
      delete statbuf;
      return fxn_ret;
    }
}
void update_server_metadata(void *userdata, char *cache_path, const char *path, struct timespec mtime){
    struct stat *stat_server = (struct stat *)malloc(sizeof(struct stat));
    int ret;
    DLOG("updata server's metadata");
    struct timespec t[2];
    t[0] = mtime;
    t[1] = mtime;
    /* cout << "【Before call rpc_utimensat, ts[0]=" << t[0].tv_sec << "; ts[1]=" \
        << t[1].tv_sec << "】" << std::endl; */
    ret = rpcCall_utimens(userdata, path, t);

    ret = rpcCall_getattr(userdata, path, stat_server);
    //cout << "【after call rpc_utimensat, server_mtime=" << stat_server->st_mtime << " 】" << std::endl;
    free(stat_server);
}
int watdfs_cli_upload(void *userdata, const char *path){
    DLOG("start upload file from client to server ......");
    struct stat *stat_client = (struct stat *)malloc(sizeof(struct stat));
    struct fuse_file_info *fi_server = (struct fuse_file_info *)malloc(sizeof(struct fuse_file_info));
    char *cache_path = get_full_path((file_state*)userdata, path);
    int ret, fxn_ret=0;

    // get file attributes on client
    ret = stat(cache_path, stat_client);

    // open the responded file on server, if call upload function, fi_server->flag must can write
    // whether file exists on server
    fi_server -> flags = O_RDWR;
    ret = rpcCall_open(userdata, path, fi_server);  // no write permission
    if(ret < 0){
        ret = rpcCall_mknod(userdata, path, stat_client -> st_mode, stat_client -> st_dev);
        ret = rpcCall_open(userdata, path, fi_server);
    }

    // read file from client to buffer
    int fh_ret = open(cache_path, O_RDONLY);
    if(ret < 0){
        fxn_ret = -errno;
    }
    char *buf = (char *)malloc((stat_client->st_size)*sizeof(char));
    ret = pread(fh_ret, buf, stat_client->st_size, 0);
    DLOG("read from client to buf is : %s", buf);
    //cout << "stat_client->st_size is ========" << stat_client->st_size;
    DLOG("pread return code ======= %d", ret);
    if(ret < 0){
        fxn_ret = -errno;
    }

    // write from buffer to server
    ret = rpcCall_truncate(userdata, path, stat_client->st_size);
    if(ret < 0){
        fxn_ret = ret;
    }
    ret = rpcCall_write(userdata, path, buf, stat_client->st_size, 0, fi_server);
    if(ret < 0){
        fxn_ret = ret;
    }
    DLOG("write from buf to server is : %s", buf);
    update_server_metadata(userdata, cache_path, path, stat_client->st_mtim);

    if(fxn_ret < 0){
        DLOG("upload to server fail");
    }else{
        DLOG("download to client success");
    }

    // free all heap
    free(stat_client);
    free(fi_server);
    free(cache_path);
    free(buf);
    return fxn_ret;
}
void update_client_metadata(void *userdata, char *cache_path, struct timespec mtime){
    struct stat *stat_client = (struct stat *)malloc(sizeof(struct stat));
    int ret;
    DLOG("updata client's metadata");
    struct timespec t[2];
    t[0] = mtime;
    t[1] = mtime;
    /*cout << "【Before call sys_utimensat, ts[0]=" << t[0].tv_sec << "; ts[1]=" \
        << t[1].tv_sec << "】" << endl; */
    ret = utimensat(0, cache_path, t, 0);

    ret = stat(cache_path, stat_client);
    //cout << "【after call sys_utimensat, client_mtime=" << stat_client->st_mtime << " 】" << endl;

    free(stat_client);
}

int watdfs_cli_download(void *userdata, const char *path){
    DLOG("start download file from server to client ......");
    struct stat *stat_server = (struct stat *)malloc(sizeof(struct stat));
    struct fuse_file_info *fi_server = (struct fuse_file_info *)malloc(sizeof(struct fuse_file_info));
    char *cache_path = get_full_path((file_state *)userdata, path);
    int ret, fxn_ret = 0;

    ret = rpcCall_getattr(userdata, path, stat_server);
    if(ret < 0){
        fxn_ret = ret;
    }

    // open both

    int fh_cli = open(cache_path, O_RDWR);
    if(fh_cli < 0){
        DLOG("file doesn't exist on client, create new file");
        ret = mknod(cache_path, stat_server->st_mode, stat_server->st_dev);
        fh_cli = open(cache_path, O_RDWR);
    }

    // truncate file on client
    ret = truncate(cache_path, stat_server -> st_size);
    if(ret < 0){
        fxn_ret = -errno;
    }

    // read file from server to buffer
    char *buf = (char *)malloc((stat_server -> st_size)*sizeof(char));
    fi_server -> flags = O_RDONLY;
    ret = rpcCall_open(userdata, path, fi_server); // 只读方式打开不可能fail
    if(ret < 0){
        fxn_ret = ret;
    }
    ret = rpcCall_read(userdata, path, buf, stat_server -> st_size, 0, fi_server);
    DLOG("read from server to buf is : %s", buf);
    if(ret < 0){
        fxn_ret = ret;
        DLOG("fail read file on server");
    }

    // write from buffer to client
    ret = pwrite(fh_cli, buf, stat_server->st_size, 0);
    if(ret < 0){
        fxn_ret = -errno;
    }
    DLOG("write from buf to client is : %s", buf);
    //cout << "stat_server->st_size is ========" << stat_server->st_size;
    DLOG("write return code ======= %d", ret);

    // update client's metadata and close server
    update_client_metadata(userdata, cache_path, stat_server->st_mtim);
    ret = rpcCall_release(userdata, path, fi_server);
    if(ret < 0){
        fxn_ret = ret;
    }
    ret = close(fh_cli);
    if(ret < 0){
        fxn_ret = -errno;
    }

    if(fxn_ret < 0){
        DLOG("download file from server to client fail");
    }else{
        DLOG("download file from server to client success");
    }


    // free all heap
    free(stat_server);
    free(fi_server);
    free(cache_path);
    free(buf);
    return fxn_ret;
}

static int push_to_server(struct file_state *userdata, const char *full_path, const char *path){

  DLOG("push data from client");
  int flag1 = O_RDWR;
  int flag2 = O_CREAT;
  int flag3 = S_IRWXU;
  int flag4 = O_RDONLY;

  int fxn_ret;
  //lock it up first
  int sys_ret = 0;//lock(path, RW_READ_LOCK);

  if (sys_ret < 0){

    DLOG("push: lock acquire error");
    return sys_ret;

  } else {

    DLOG("push: gets the lock");

    int fxn_ret = sys_ret;
    struct fuse_file_info *fi = new struct fuse_file_info;
    fi->flags = flag1;

    struct stat *statbuf = new struct stat;
    mode_t mt = statbuf->st_mode;
    dev_t dt = statbuf->st_dev;

    int ret_code = stat(full_path, statbuf);

    ret_code = rpcCall_open((void *)userdata, path, fi);

    // kee looping
    if (ret_code < 0){

      // first create the file
      ret_code = rpcCall_mknod((void *)userdata, path, mt, dt);
      // open the created file. this hsould succeed, othwrwise, loop again
      ret_code = rpcCall_open((void *)userdata, path, fi);
    }


    size_t size = statbuf->st_size; // might need to cast to offset type

    // open local file
    sys_ret = open(full_path, flag4); //TODO???

    if (sys_ret < 0){
      DLOG("push error");
      delete statbuf;
      delete fi;
      //unlock(path, RW_READ_LOCK);
      return -errno;
    }
    // loop until a new file is created
    // while (sys_ret < 0) {
    //   DLOG("create a new file now");
    //   // trigger sys call for mknod
    //   mknod(full_path, statbuf->st_mode, statbuf->st_dev);
    //
    //   sys_ret = open(full_path, flag1);
    // }


    // struct fileMetadata *target = (*((openFiles *) userdata))[path];
    fi->flags = flag1;
    // (((userdata->openFiles)[full_path]).client_mode) = local_fh_retcode;

    // // step1: truncate local file
    // rpc_ret = truncate(full_path, (off_t)size);
    //
    // if (rpc_ret < 0){
    //   DLOG("download error")
    //   free(full_path);
    //   delete statbuf;
    //   delete fi;
    //   unlock(path, RW_READ_LOCK);
    //   return -errno;
    // }

    //DLOG(.*)
    // TODO???? ret_code = rpc_open(userdata, path, fi);
    // read the file from server
    char * buf = (char *) malloc(((off_t) size) * sizeof(char));

    // write from server to local
    ret_code = pread(sys_ret, buf, size, 0);

    if (ret_code < 0){
      DLOG("push error");
      //unlock(path, RW_READ_LOCK);
      free(buf);
      delete statbuf;
      return -errno;
    }

    // step1: truncate local file
    ret_code = rpcCall_truncate((void *)userdata, path, (off_t) size);

    if (ret_code < 0){
      DLOG("push error");
      delete statbuf;
      delete fi;
      //unlock(path, RW_READ_LOCK);
      return ret_code;
    }

    ret_code = rpcCall_write((void*)userdata, path, buf, (off_t) size, 0, fi);
    if(ret_code < 0){
      DLOG("push error");
      delete statbuf;
      delete fi;
      //unlock(path, RW_READ_LOCK);
      return ret_code;
    }

    // update metadata
    // target->client_mode = local_fh_retcode;// server
    // target->server_mode = fi->fh;// local

    // update Tclient = Tserver and Tc = current time
    struct timespec ts[2];

    ts[0] = (struct timespec)(statbuf->st_mtim);
    ts[1] = (struct timespec)(statbuf->st_mtim);

    int dirfd = 0;
    int flag = 0;
    // update the timestamps of a file by calling utimensat
    ret_code = rpcCall_utimens((void *)userdata, path, ts);
    if (ret_code < 0) {
      DLOG("push error1");
      delete statbuf;
      delete fi;
      //unlock(path, RW_READ_LOCK);
      return ret_code;
    }

    ret_code = rpcCall_getattr((void *)userdata, path, statbuf);
    if (ret_code < 0) {
      DLOG("push error2");
      delete statbuf;
      delete fi;
      //unlock(path, RW_READ_LOCK);
      return ret_code;
    }

    // //TODO??? STAT?
    // DLOG("download: metadata updated");
    // rpc_ret = rpcCall_release(userdata, path, fi);
    // if(rpc_ret < 0){
    //   DLOG("download error")
    //   unlock(path, RW_READ_LOCK);
    //   free(buf);
    //   delete statbuf;
    //   free(full_path);
    //   return sys_ret;
    // }
    //
    // // close file locally
    // ret = close(sys_ret);
    // if(ret < 0){
    //   DLOG("download error")
    //   unlock(path, RW_READ_LOCK);
    //   free(buf);
    //   delete statbuf;
    //   free(full_path);
    //   return -errno;
    // }

    // ret_code = unlock(path, RW_READ_LOCK);
    //
    // if (ret_code < 0){
    //   DLOG("push error3");
    //   free(buf);
    //   // delete ts;
    //   delete statbuf;
    //   return sys_ret;
    // }

    DLOG("Push succeed");


    free(buf);
    delete fi;

    delete statbuf;
    return fxn_ret;
  }
}

// w =1, r = 0
int freshness_check(struct file_state *userdata, const char *full_path, const char *path) {

    DLOG("checking freshness");

    int sys_ret, fxn_ret, ret_code;
    struct fileMetadata file_meta = (userdata->openFiles)[std::string(full_path)];
    time_t T = userdata->cacheInterval;

    // get T_client and T_server
    struct stat *statClient = new struct stat;
    struct stat *statServer = new struct stat;

    sys_ret = rpcCall_getattr((void *)userdata, path, statServer);

    if (sys_ret < 0){
        free(statClient);
        free(statServer);
        return sys_ret;
    }
    DLOG("checking freshness: getattr call good");
    fxn_ret = sys_ret;

    time_t T_client = statClient->st_mtime;
    time_t T_server = statServer->st_mtime;

    bool server_client_diff = statClient->st_mtime ==  statServer->st_mtime;
    bool within_interval = -1 * (file_meta.tc - time(0)) < T;

    // not needed any more
    free(statClient);
    free(statServer);

    if (server_client_diff || within_interval) {
      DLOG("checking freshness: cond true");
      // update the open time to curr
      (userdata->openFiles)[std::string(full_path)].tc = time(0);
      return 1;
    } else {
      return 0;
    }
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
    struct file_state * userdata = new struct file_state;
    // userdata->cachePath=  new std::map<std::string, fileMetadata*>(); //TODO: free it
    userdata->cacheInterval = cache_interval;
    // allocate memory for path
    int str_len = strlen(path_to_cache) + 1;
    userdata->cachePath = (char *)malloc(str_len);
    strcpy(userdata->cachePath, path_to_cache);

    // Return pointer to global state data.
    return (void *) userdata;
}

//TODO: might need to free path as well
void watdfs_cli_destroy(void *userdata) {


    int destoryRet = rpcClientDestroy();

    if (!destoryRet) DLOG("Client Destory Success");
    else DLOG("Client Destory Fail");

    // for(auto it : (*(openFiles *)userdata)) {
    //   delete it.second;
    // }

    free(((struct file_state *)userdata)->cachePath);
    // delete userdata;
    userdata = NULL;
}


bool is_fresh(void *userdata, const char *path){    // file is confirmed on both of client and server
    //cout << "*********** current time is:  " << time(0) << "  *************"<< std::endl;
    char *cache_path = get_full_path((struct file_state*)userdata, path);
    struct file_state *user = (file_state *)userdata;
    std::string s_cache_path(cache_path);
    int ret;
    if(time(0)-(user->openFiles)[s_cache_path].tc < user->cacheInterval){     // cache time is less then interval time, then it's fresh
        //cout << "【T - tc = " << time(0)-(user->open_file)[s_cache_path].tc << "】"<< std::endl;
        DLOG("【T - tc < t, fresh】");
        return true;
    }else{
        struct stat *stat_client = (struct stat *)malloc(sizeof(struct stat));
        struct stat *stat_server = (struct stat *)malloc(sizeof(struct stat));

        ret = rpcCall_getattr(userdata, path, stat_server);
        if(ret < 0){
            DLOG("get attr on server fail");
        }
        ret = stat(cache_path, stat_client);
        if(ret < 0){
            DLOG("get attr on client fail");
        }

        if(stat_client -> st_mtime == stat_server->st_mtime){ // modify is the same on both of client and server
            time_to_curr(userdata, cache_path);
            free(stat_client);
            free(stat_server);
            DLOG("【T_server = T_client, fresh】");
            return true;
        }
    }
    DLOG("【The file is not fresh】");
    return false;
}
bool is_open(void *userdata, char *cache_path){
    struct file_state *user = (file_state *)userdata;
    std::string s_cache_path(cache_path);
    if((user->openFiles).find(s_cache_path) != (user->openFiles).end()){
        DLOG("now file is open");
        return true;
    }else{
        DLOG("now file is close");
        return false;
    }
}
// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf){
    int flag1 = O_RDONLY;
    int flag2 = O_ACCMODE;
    int fxn_ret = 0;
    int sys_ret = 0;

    DLOG("getattr triggered");

    char *full_path = get_full_path((struct file_state *)userdata, path);
    std::string p = std::string(full_path);

    // validate if file open, and prepare for downloading the data to client
    if (!is_file_open((struct file_state *)userdata, full_path)) {
      DLOG("getattr triggered111");
      struct stat *statbuf_tmp = new struct stat;
      DLOG("getattr triggered1111");
      int ret_code = rpcCall_getattr(userdata, path, statbuf_tmp);
      DLOG("getattr triggered111111");
      if (ret_code < 0) {
        DLOG("error in getattr (1)");
        free(statbuf_tmp);
        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        fxn_ret = ret_code;
        return fxn_ret;
      }
      DLOG("getattr triggered1");
      // download data to client
      ret_code = watdfs_cli_download(userdata, path);;
      sys_ret = open(full_path, flag1);
      // update metadata to clients.
      ret_code = stat(full_path, statbuf);
      //TODO: need to release?
      //close local file
      fxn_ret = close(sys_ret);
      if(fxn_ret < 0){
        DLOG("error in getattr (2)");
        free(statbuf_tmp);
        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        fxn_ret = -errno;
        return fxn_ret;
      }
      delete statbuf_tmp;
    } else {
      DLOG("getattr triggered5");
      struct stat *statbuf_tmp = new struct stat;
      //read only the file, file already opened
      if (flag1 == (((struct file_state*)userdata)->openFiles)[p].client_mode &
            flag2 && !is_fresh(userdata, path)) {
        // read only, check freshness, download file
        DLOG("stat downloading file");

        int ret_code = watdfs_cli_download(userdata, path);;

        if (ret_code < 0) {
          DLOG("error in getattr (3)");
          memset(statbuf, 0, sizeof(struct stat));
          free(full_path);
          free(statbuf_tmp);
          fxn_ret = ret_code;
          return fxn_ret;
        }
        time_t curr_time = time(0);
        ((((struct file_state*)userdata)->openFiles)[p]).tc = curr_time;
      }
      DLOG("getattr triggered6");
      // sys call to stat
      int ret_code = stat(full_path, statbuf);
      if(ret_code < 0){
        DLOG("error in getattr (4)");

        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        free(statbuf_tmp);
        fxn_ret = -errno;
        return fxn_ret;
      }
      // TODO: need to close the file?
      delete statbuf_tmp;
    }
    free(full_path);

    DLOG("getattr finished");
    // FINAL RETURN THE HANDLER CODE
    return fxn_ret;
    // struct stat *stat_server = (struct stat *)malloc(sizeof(struct stat));
    // int ret, fxn_ret = 0;
    // char *cache_path = get_full_path((file_state *)userdata, path);
    //
    // // check open
    // if(!is_open(userdata, cache_path)) {   // if file doesn't open, then transfer file from server to client
    //     DLOG("file not open");
    //     ret = rpcCall_getattr(userdata, path, stat_server);
    //     if(ret < 0){   // don't exist on server
    //         DLOG("file doesn't on server");
    //         fxn_ret = ret;
    //     }else{
    //         ret = watdfs_cli_download(userdata, path);
    //         DLOG("watdfs_cli_download return code ===== %d", ret);
    //         int fh_ret = open(cache_path, O_RDONLY);
    //         ret = stat(cache_path, statbuf);  // 文件已在本地存在，所以不会失败
    //         ret = close(fh_ret);
    //         if(ret < 0){
    //             fxn_ret = -errno;
    //         }
    //     }
    // }else {
    //     // check file flag
    //     // file open in read mode must check fresh, in write mode can't check fresh
    //     if (get_flag(userdata, cache_path) == O_RDONLY) {
    //         // check freshness
    //         if (!is_fresh(userdata, path)) {
    //             ret = watdfs_cli_download(userdata, path); // 不可能失败，因为文件一定存在，并且以在server上以只读的形式打开
    //             // 已经打开的文件，fh没有从open_file里删掉，也没有可以关闭文件，相当于没有关闭，因此没必要重新打开
    //             DLOG("watdfs_cli_download return code ===== %d", ret);
    //             if(ret < 0){
    //                 fxn_ret = ret;
    //             }else{
    //                 reset_tc(userdata, cache_path);
    //             }
    //         }
    //     }
    //     ret = stat(cache_path, statbuf);
    //     DLOG("stat return code ===== %d", ret);
    //     if(ret < 0){
    //         fxn_ret = -errno;
    //     }
    // }
    //
    // if(fxn_ret < 0){
    //     DLOG("watdfs_cli_getattr fail");
    //     memset(statbuf, 0, sizeof(struct stat));
    // }
    //
    // free(stat_server);
    // free(cache_path);
    // return fxn_ret;
}

int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi) {

    DLOG("NEW: Received fgetattr call from local client...");

    int flag1 = O_RDONLY;
    int flag2 = O_ACCMODE;
    int fxn_ret = 0;
    int sys_ret = 0;

    DLOG("getattr triggered");

    char *full_path = get_full_path((struct file_state *)userdata, path);

    std::string p = std::string(full_path);

    // validate if file open, and prepare for downloading the data to client
    if (!is_file_open((struct file_state *)userdata, full_path)) {
      struct stat *statbuf_tmp = new struct stat;
      int ret_code = rpcCall_fgetattr(userdata, path, statbuf_tmp, fi);
      if (ret_code < 0) {
        // cannot find on the server
        DLOG("error in fgetattr (1)");
        free(statbuf_tmp);
        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        fxn_ret = ret_code;
        return fxn_ret;
      }
      // download data to client
      ret_code = watdfs_cli_download(userdata, path);
      sys_ret = open(full_path, flag1);
      // get file info
      ret_code = fstat(sys_ret, statbuf);
      //TODO: need to release?
      //close local file
      fxn_ret = close(sys_ret); // close through open's code
      if(fxn_ret < 0){
        DLOG("error in fgetattr (2)");
        free(statbuf_tmp);
        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        fxn_ret = -errno;
        return fxn_ret;
      }
      delete statbuf_tmp;
    } else {
      struct stat *statbuf_tmp = new struct stat;

      //read only the file, file already opened
      if ((flag1 == ((((struct file_state*)userdata)->openFiles)[p].client_mode &
            flag2)) && (!is_fresh(userdata, path))) {
        // read only, check freshness, download file
        DLOG("stat downloading file");

        int ret_code = watdfs_cli_download(userdata, path);

        if (ret_code < 0) {
          DLOG("error in fgetattr (3)");

          memset(statbuf, 0, sizeof(struct stat));
          free(full_path);
          free(statbuf_tmp);
          fxn_ret = ret_code;
          return fxn_ret;
        }
        time_t curr_time = time(0);
        ((((struct file_state*)userdata)->openFiles)[p]).tc = curr_time;
      }
      // sys call to stat
      int serverM = ((((struct file_state*)userdata)->openFiles)[p]).server_mode;
      int ret_code = fstat(serverM, statbuf);
      if(ret_code < 0){
        DLOG("error in getattr (4)");

        memset(statbuf, 0, sizeof(struct stat));
        free(full_path);
        free(statbuf_tmp);
        fxn_ret = -errno;
        return fxn_ret;
      }
      delete statbuf_tmp;
      // TODO: need to close the file?

    }
    DLOG("fgetarrt finished");
    free(full_path);
    // FINAL RETURN THE HANDLER CODE
    return fxn_ret;
}
int mknod_update(void *userdata, const char *full_path, const char *path, int flag,
                  mode_t mode, dev_t dev) {
  int fxn_ret = 0;
  int ret_code = 0;
  if(flag == O_RDONLY) return -EMFILE;

  ret_code = mknod(full_path, mode, dev);
  if (ret_code < 0) return -errno;

  if(!is_fresh(userdata, path)){
      ret_code = watdfs_cli_upload(userdata, path);
      if(ret_code < 0) return ret_code;
      time_to_curr(userdata, full_path);
  } else {
    return 0;
  }
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {

    DLOG("NEW: Received mknod call from local client...");

    char *full_path = get_full_path((struct file_state *)userdata, path);

    //TODO???: chekc if file is open
    int fxn_ret = 0;

    if (!is_file_open((struct file_state *)userdata, full_path)) {
      int sys_ret = mknod(full_path, mode, dev);
      if (sys_ret < 0)  fxn_ret = -EINVAL;
      else fxn_ret = watdfs_cli_upload(userdata, path);
      fxn_ret = fxn_ret < 0 ? fxn_ret : 0;
    } else {
      int fl = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].client_mode & O_ACCMODE;
      fxn_ret = mknod_update(userdata, full_path, path, fl, mode, dev);
    }

    // Clean up the memory we have allocated.
    free(full_path);

    DLOG("DONE: mknod: return code is %d", fxn_ret);

    return fxn_ret;
}


int file_open_load(void *userdata, const char * full_path, const char *path, struct fuse_file_info *fi) {
  int ret_code = rpcCall_open(userdata, path, fi);
  int fxn_ret = (ret_code < 0) ? ret_code : 0;
  int ret_code2 = watdfs_cli_download(userdata, path);
  fxn_ret = (ret_code2 < 0) ? ret_code2 : 0;
  DLOG("open: new file is created and loaded into client");
  return fxn_ret;
}
int open_local_file(void *userdata, char *cache_path, int flags){
    std::string s_cache_path(cache_path);
    int ret;
    if(is_file_open((struct file_state *)userdata, cache_path)){
        DLOG("file is open on client, can't open again");
        return -EMFILE;
    }
    ret = open(cache_path, flags);
    if(ret < 0){
        DLOG("open local file fail");
        return -errno;
    }else{
        struct file_state *user = (file_state *)userdata;
        (user->openFiles)[s_cache_path].client_mode = flags;
        (user->openFiles)[s_cache_path].server_mode = ret;
        (user->openFiles)[s_cache_path].tc = time(0);
        DLOG("open file and update metadata on client success");
        return 0;
    }
}

int open_cond(void *userdata, int code, const char *path, struct fuse_file_info *fi, int f) {
  int fxn_ret = 0;
  if (fi->flags != f) {
    return code;
  }
  int ret_code = rpcCall_open(userdata, path, fi);
  if (ret_code < 0) fxn_ret = ret_code;
  ret_code = watdfs_cli_download(userdata, path);
  if (ret_code < 0) return ret_code;
  else return fxn_ret;
}

int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi) {

    char *full_path = get_full_path((struct file_state *)userdata, path);
    int flag1 = O_CREAT;
    // if alreadfy opened, error
    std::string p = std::string(full_path);
    // validate if file open, and prepare for downloading the data to client
    if(is_file_open((struct file_state *)userdata, full_path)){
      DLOG("already open error");
      free(full_path);
      return -EMFILE;
    }

    int ret_code = 0;
    int fxn_ret = 0;

    struct stat *statbuf = new struct stat;

    // check whether file exists on server
    ret_code = rpcCall_getattr(userdata, path, statbuf);

    if (ret_code < 0) {
      fxn_ret = open_cond(userdata, ret_code, path, fi, O_CREAT);
    } else {
      ret_code = watdfs_cli_download(userdata, path);
      if (ret_code < 0) fxn_ret = ret_code;
    }

    if (fxn_ret){
      // record the file on the map
      ret_code = open(full_path, fi->flags);
      if (ret_code < 0) {
        free(full_path);
        free(statbuf);
        return -errno;
      }
      DLOG("open: record this file now");
      // update metadata
      struct fileMetadata newFile = {fi->flags, ret_code, time(0)};
      (((struct file_state*)userdata)->openFiles)[std::string(full_path)] = newFile;
      fxn_ret = 0;
    } else {
      fxn_ret = -errno;
    }

    free(full_path);
    free(statbuf);
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path,
                       struct fuse_file_info *fi) {

    int sys_ret = 0;
    int ret_code = 0;
    int fxn_ret = 0;

    char *full_path = get_full_path((struct file_state *)userdata, path);

    DLOG("Received release rpcCall from local client...");

    int file_flag = (((struct file_state *)userdata)->openFiles)[std::string(full_path)].client_mode;

    if (!(readonly == (file_flag & O_ACCMODE))) {
      // not read only, push the updates to server
       sys_ret = watdfs_cli_upload(userdata, path);
       if (sys_ret < 0) return sys_ret;
    }

   // ret_code = rpcCall_release(userdata, path, fi);
   //
   // if (ret_code < 0){
   //   fxn_ret = ret_code;
   //   return fxn_ret;
   // }

   // now remove the files from openFiles and close it locally

   if(!is_file_open((file_state*)userdata, full_path)){
       DLOG("not open error detected in release");
       fxn_ret = -ENOENT;
   } else {
     sys_ret = close((((struct file_state*)userdata)->openFiles)[std::string(full_path)].server_mode);

     if (sys_ret < 0) fxn_ret = -errno;
     else (((struct file_state*)userdata)->openFiles).erase(std::string(full_path));
  }
  free(full_path);
  fxn_ret = fxn_ret < 0 ? fxn_ret : 0;
  return fxn_ret;
}


// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {

    DLOG("Received read rpcCall from local client...");

    int fxn_ret = 0;
    char *full_path = get_full_path((struct file_state *)userdata, path);

    if(!is_file_open((struct file_state *)userdata, full_path)){
      DLOG("error in read");
      free(full_path);
      return -EPERM;
    }
    int clientM = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].client_mode & O_ACCMODE;
    if (clientM != O_RDONLY) {
      int serverM = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].server_mode;

      fxn_ret = pread(serverM, buf, size, offset);
      free(full_path);
      return fxn_ret;
    }

    int ret_code = is_fresh(userdata, path);
    if (ret_code == 0) {
      ret_code = watdfs_cli_download(userdata, path);
      if (ret_code < 0) return -EPERM;
      // reset the time to current
      (((struct file_state*)userdata)->openFiles)[std::string(full_path)].tc = time(0);
    }

    int serverM = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].server_mode;

    fxn_ret = pread(serverM, buf, size, offset);
    free(full_path);

    return fxn_ret;
}

void check_fresh_then_load(void *userdata, const char * full_path, const char *path, int *code) {
  int fresh_or_not = is_fresh(userdata, path);
  int fxn_ret = 0;
  if (fresh_or_not == 0) {
    fxn_ret = watdfs_cli_upload(userdata, path);
    // set time to current
    (((struct file_state*)userdata)->openFiles)[std::string(full_path)].tc = time(0);
    if (fxn_ret < 0) *code = fxn_ret;
  }

}

int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {

    DLOG("Received write rpcCall from local client...");

    int fxn_ret = 0;
    int ret_code = 0;
    char *full_path = get_full_path((struct file_state *)userdata, path);

    if(!is_file_open((struct file_state *)userdata, full_path)){
      DLOG("error in write");
      free(full_path);
      return -EPERM;
    }

    struct stat *statbuf = new struct stat;

    int serverM = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].server_mode;

    ret_code = pwrite(serverM, buf, size, offset);

    if (ret_code < 0) fxn_ret = -errno;
    else check_fresh_then_load(userdata, full_path, path, &fxn_ret);

    // free memory
    free(full_path);
    free(statbuf);

    fxn_ret = fxn_ret < 0 ? fxn_ret : ret_code;
    return fxn_ret;
}

int truncate_update(void *userdata, int flag, const char* full_path, const char* path, off_t newsize) {
  if (flag != O_RDONLY) {
    int ret_code = truncate(full_path, newsize);
    if (ret_code < 0) {
      return -errno;
    }
    if (!is_fresh(userdata, path)) {
      ret_code = watdfs_cli_upload(userdata, path);
      // set time to current
      (((struct file_state*)userdata)->openFiles)[std::string(full_path)].tc = time(0);
      if (ret_code < 0) return ret_code;
    } else {
      return 0;
    }

  } else {
    return -errno;
  }
}

int download_open(void *userdata, int flag, const char* full_path, const char* path, off_t newsize) {
  int ret_code = watdfs_cli_download(userdata, path);
  int ret_code2 = open(full_path, O_RDWR);
  int fxn_ret = 0;
  ret_code = truncate(full_path, newsize);
  if( ret_code < 0) {
      DLOG("truncate on client fail");
      fxn_ret = -errno;
  }
  close(ret_code2);
  return fxn_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {

  DLOG("Received truncate rpcCall from local client...");

  int fxn_ret = 0;
  int ret_code = 0;

  char *full_path = get_full_path((struct file_state *)userdata, path);
  struct stat *statbuf = new struct stat;

  std::string p = std::string(full_path);
  if (!is_file_open((struct file_state *)userdata, full_path)) {
    ret_code = rpcCall_getattr(userdata, path, statbuf);
    if (ret_code < 0) {
      fxn_ret = ret_code;
      free(statbuf);
      DLOG("error in truncate");
      return fxn_ret;
    }
    int clientM = (((struct file_state *)userdata)->openFiles)[std::string(full_path)].client_mode & O_ACCMODE;
    fxn_ret = download_open(userdata, clientM, full_path, path, newsize);
  } else {
    int flag = (((struct file_state*)userdata)->openFiles)[std::string(full_path)].client_mode & O_ACCMODE;
    fxn_ret = truncate_update(userdata, flag, full_path, path, newsize);

  }
  free(statbuf);

  return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {

  DLOG("Received fsync rpcCall from local client...");
  char *full_path = get_full_path((struct file_state *)userdata, path);

  int ret_code = 0;
  int fxn_ret = 0;
  ret_code = watdfs_cli_upload(userdata, path);

  if (ret_code < 0) {
    DLOG("fsyn failed to upload data");
    fxn_ret = ret_code;
  }

  //update time
  // set time to current
  (((struct file_state*)userdata)->openFiles)[std::string(full_path)].tc = time(0);

  DLOG("DONE: sync: return code is %d", fxn_ret);
  free(full_path);
  // Finally return the value we got from the server.
  return fxn_ret;
}
int utimens_update(void *userdata, int flag, const char* full_path, const char* path, const struct timespec ts[2]) {
  if (flag != O_RDONLY) {
    int ret_code = utimensat(0, full_path, ts, 0);
    if (ret_code < 0) {
      return -errno;
    }
    if (!is_fresh(userdata, path)) {
      // set time to current
      ret_code = watdfs_cli_upload(userdata, path);
      if (ret_code < 0) return ret_code;
      (((struct file_state*)userdata)->openFiles)[std::string(full_path)].tc = time(0);

    } else {
      return 0;
    }

  } else {
    return -EMFILE;
  }
}
// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path,
     const struct timespec ts[2]) {

  // Called to release a file.
  // SET UP THE RPC CALL
  DLOG("Received utimens rpcCall from local client...");
  int flag = O_RDWR;
  char *full_path = get_full_path((struct file_state *)userdata, path);
  int ret_code = 0;
  int fxn_ret = 0;
  int ret_code2 = 0;
  struct stat *statbuf = new struct stat;
  // check open
  std::string p = std::string(full_path);
  if (!is_file_open((struct file_state *)userdata, full_path)) {
      ret_code = rpcCall_getattr((void *)userdata, path, statbuf);
      if(ret_code < 0) {
        free(full_path);
        free(statbuf);
        return ret_code;
      }

      ret_code = watdfs_cli_download(userdata, path);

      int ret_code2 = open(full_path, flag);

      if(ret_code2 < 0) {
        free(full_path);
        free(statbuf);
        DLOG("error in utimens(1)");
        return ret_code2;
      }
      int offsets = 0;

      ret_code = utimensat(0, full_path, ts, offsets);
      if(ret_code < 0){
        free(full_path);
        free(statbuf);
        DLOG("error in utimens(2)");
        return -errno;
      }
      ret_code = close(ret_code2);
      if(ret_code < 0){
        free(full_path);
        free(statbuf);
        DLOG("error in utimens(3)");
        return -errno;
      }
  } else {
    int flag_client = (((struct file_state *)userdata)->openFiles)[std::string(full_path)].client_mode & O_ACCMODE;
    fxn_ret = utimens_update(userdata, flag, full_path, path, ts);
  }

  free(statbuf);
  free(full_path);
  return fxn_ret;
}
