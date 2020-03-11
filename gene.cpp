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

#include <time.h>

#include "watdfs_client_remote.cpp"
#include "watdfs_client_utils.cpp"



// You may want to include iostream or cstdio.h if you print to standard error.

// We need to operate on the path relative to the the client_persist_dir.
// This function returns a path that appends the given short path to the
// server_persist_dir. The character array is allocated on the heap, therefore
// it should be freed after use.

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval) {
    int ret_code;

    std::cerr <<"cli init called"<< std::endl;

    ret_code = rpcClientInit();

    struct global_state *userdata = new struct global_state;

    userdata->client_persist_dir = (char *)malloc(strlen(path_to_cache)+1);
    strcpy(userdata->client_persist_dir, path_to_cache);
    userdata->cache_interval = cache_interval;

    //std::cerr <<"init finished"<< std::endl;

    return (void *) userdata;
}

void watdfs_cli_destroy(void *userdata) {
    // You should clean up your userdata state here.
    // You should also tear down the RPC library by calling rpcClientDestroy.
    rpcClientDestroy();

    userdata = (struct global_state *) userdata;

    // iterating over all value of opened_files
    std::map<std::string, struct file_metadata *>:: iterator itr;
    for (itr = ((struct global_state *)userdata)->opened_files.begin(); itr != ((struct global_state *)userdata)->opened_files.end(); itr++){
        //itr->second->file_statistics;
        delete itr->second;
    }
    free(((struct global_state *)userdata)->client_persist_dir);
    delete userdata;
}

// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf){
    int flag = O_RDONLY;

    std::cerr <<"cli getattr called"<< std::endl;
    int fxn_ret = 0;
    if (!file_opened(userdata, path)){// file not opened at the client side, should be closed
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

        char *full_path = get_full_path(userdata, path);
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
        int utils_ret = 0;
        utils_ret = read_freshness_check(userdata, path);

        set_validate_time(userdata, path);
        if (utils_ret < 0){
            return utils_ret;
        }

        char *full_path = get_full_path(userdata, path);
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
                        struct fuse_file_info *fi){
    //std::cerr <<"cli getattr called"<< std::endl;
    int flag = O_RDONLY;

    int fxn_ret = 0;
    if (!file_opened(userdata, path)){// file not opened at the client side, should be closed
        std::cerr <<"file has not been opened"<< std::endl;

        /******* check if a file exist on the server *******/
        if (!check_if_file_exist_on_server(userdata, path)){// true if file does not exist
            std::cerr << "in getattr file does not exist on server" << std::endl;
            return -2;
        }
        /******* check if a file exist on the server *******/

        struct fuse_file_info* file_info = new struct fuse_file_info;
        file_info->flags = O_RDWR;

        int dfs_ret = watdfs_cli_open(userdata, path, file_info);
        if (dfs_ret < 0){
            delete file_info;
            return dfs_ret;
        }

        // MAKE THE system call
        int sys_ret = fstat((((struct global_state *)userdata)->opened_files[path]->fh_client_local), statbuf);

        if (sys_ret < 0) {
            fxn_ret = -errno;
            memset(statbuf, 0, sizeof(struct stat));
        }else{
            fxn_ret = sys_ret;
        }

        // Clean up the full path, it was allocated on the heap.
        //std::cerr <<"system return "<< sys_ret << std::endl;
        std::cerr <<"client getattr finished"<< std::endl;

        sys_ret = close(((struct global_state *)userdata)->opened_files[path]->fh_client_local);
        ((struct global_state *)userdata)->opened_files.erase(path);
        fxn_ret = sys_ret;

        return fxn_ret;
    }else{// file already opened
        int utils_ret = 0;
        utils_ret = read_freshness_check(userdata, path);
        if (utils_ret < 0){
            return utils_ret;
        }
        set_validate_time(userdata, path);
        char *full_path = get_full_path(userdata, path);
        // MAKE THE system call
        int sys_ret = fstat((((struct global_state *)userdata)->opened_files[path]->fh_client_local), statbuf);

        if (sys_ret < 0) {
            fxn_ret = -errno;
            memset(statbuf, 0, sizeof(struct stat));
        }else{
            fxn_ret = sys_ret;
        }

        //std::cerr <<"file size "<< statbuf->st_size << std::endl;

        // Clean up the full path, it was allocated on the heap.
        free(full_path);
        //std::cerr <<"system return "<< sys_ret << std::endl;
        //std::cerr <<"client getattr finished"<< std::endl;

        return fxn_ret;
    }
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {
    int flag = O_WRONLY;
    // no if condition for whether
    // *********************** remote first ***********************
    std::cerr << "watdfs cli mknod called" << std::endl;
    // if a file is on the server, there must be a copy in the local if we open it, the only situation
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
    //std::cerr <<"Client mknod"<< std::endl;
    //std::cerr << "mode" << mode << "dev" << dev << std::endl;
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
        //std::cerr << "error code:" << ret_code << std::endl;
#endif
        fxn_ret = ret_code;

        if (ret_code < 0){
            free(args);
            return fxn_ret;
        }
    }

    // Clean up the memory we have allocated.
    free(args);

    //*********************** the remote part is done ***********************
    char *full_path = get_full_path(userdata, path);

    int sys_ret = mknod(full_path, mode, dev);

    // HANDLE THE RETURN
    // The integer value watdfs_cli_mknod will return.
    if (sys_ret < 0) {//rpc_call failed
        fxn_ret = -EINVAL;
    } else {
        fxn_ret = sys_ret;
    }
    free(full_path);

    //std::cerr << "watdfs client mknod " << fxn_ret << std::endl;

    // Finally return the value we got from the server.
    return fxn_ret;
}

// open should assume there always exists a file on the server, because even a file does not exist on the server, getattr will return -2, and then mknod will be called
int watdfs_cli_open(void *userdata, const char *path, struct fuse_file_info *fi) {
    std::cerr <<"cli open called"<< std::endl;

    int FUSE_flag, client_flag, server_flag;

    FUSE_flag = fi->flags;

    if ( (FUSE_flag & O_ACCMODE) == O_RDONLY ){
        client_flag = O_RDWR;
        server_flag = O_RDONLY;
    }else{
        client_flag = O_RDWR;
        server_flag = O_RDWR;
    }

    // open assumes the file alread exists on the server
    // avoid the client open the same file
    if (file_opened(userdata, path)){
        std::cerr << "EMFILE" << std::endl;
        return EMFILE;
    }
    // ************ remote **************

    // ********************* local flag ******************
    ((global_state *) userdata)->opened_files[path] = new struct file_metadata;

    fi->flags = server_flag;// for the flags on server, store the flags


    // You should fill in fi->fh.
    // filled by server

    // mknod has 4 arguments.
    int num_args = 3;

    // Allocate space for the output arguments.
    void **args = (void**) malloc(num_args*sizeof(void*));
    int arg_types[num_args + 1];
    int pathlen = strlen(path) + 1;

    // The first argument
    arg_types[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void*)path;

    arg_types[1] = (1 << ARG_OUTPUT) |(1 << ARG_INPUT) | (1 << ARG_ARRAY)|  (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void*) fi;// fh has been setted here

    arg_types[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16) ; // not array, last 2 bytes 0
    int ret_code;
    args[2] = (void*) & ret_code;

    arg_types[3] = 0;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"open", arg_types, args);

    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = rpc_ret;
        free(args);
        return fxn_ret;
    } else {//rpcCall succeed
        if (ret_code >= 0){// the file is correctly opened
            fxn_ret = 0;
        }else{
            fxn_ret = ret_code;
            free(args);
            return fxn_ret;
        }
    }

    free(args);
    /************** end of remote open ***************/
    std::cerr << "* remote fh " << fi->fh << std::endl;

    // local open
    fi->flags = client_flag;

    int dfs_result = download(userdata, path, fi);

    fi->flags = server_flag;
    fxn_ret = (dfs_result < 0) ? dfs_result : 0;

    set_validate_time(userdata, path);
    // Finally return the value we got from the server.
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path, struct fuse_file_info *fi) {
    int dfs_result, sys_ret, fxn_ret;
    std::cerr <<"cli release called"<< std::endl;
    if ( (fi->flags & O_ACCMODE) != O_RDONLY ){
        sys_ret = upload(userdata, path, fi);
        if (sys_ret < 0){
            return sys_ret;
        }
    }

    dfs_result = watdfs_cli_release_remote(userdata, path, fi);
    if (dfs_result < 0){
        return dfs_result;
    }
    // **************** the file has been closed at the server side ****************

    struct file_metadata *file_info = ((global_state*)userdata)->opened_files[path];
    sys_ret = close(file_info->fh_client_local);// the local file has been closed
    if (sys_ret < 0) {
        fxn_ret = -errno;
    }else{
        ((global_state*)userdata)->opened_files.erase(path);
    }
    fxn_ret = sys_ret;
    return fxn_ret;

}

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    int utils_ret = 0;
    utils_ret = read_freshness_check(userdata, path);
    set_validate_time(userdata, path);
    if (utils_ret < 0){
        return utils_ret;
    }
    return write_from_local_file_to_buffer(userdata, path, buf, size, offset, fi);
}

int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi) {
    int flag = O_WRONLY;
    int sys_ret, fxn_ret;

    sys_ret = write_from_buffer_to_local_file(userdata, path, buf, size, offset);
    if (sys_ret < 0){
        return sys_ret;
    }

    write_freshness_check(userdata, path);
    set_validate_time(userdata, path);

    fxn_ret = sys_ret;
    std::cerr <<"cli write called fh  "<< ((global_state*)userdata)->opened_files[path]->fh_client_local << std::endl;
    return fxn_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {
    int flag = O_WRONLY;
    int sys_ret = 0;
    // Change the file size to newsize.
    char *full_path = get_full_path(userdata,path);

    //sys call
    sys_ret = truncate(full_path, newsize);

    // local change first then check
    sys_ret = write_freshness_check(userdata, path);
    if (sys_ret < 0){
        return sys_ret;
    }

    // HANDLE THE RETURN
    // check the file handler
    int fxn_ret = sys_ret;

    free(full_path);
    return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi) {
    // Make the fsync system call
    int sys_ret = 0;

    // write the file back to server
    sys_ret = upload(userdata, path, fi);
    if (sys_ret < 0){
        return sys_ret;
    }

    int fxn_ret = sys_ret;

    set_validate_time(userdata, path);
    // Finally return the value we got from the server.
    return fxn_ret;
}

// CHANGE METADATA
//(if changing the file access time, it does not need to be written to the server).
int watdfs_cli_utimens(void *userdata, const char *path,
                       const struct timespec ts[2]) {
    int flag = -1;
    std::cerr <<"utimens"<< std::endl;
    char * full_path = get_full_path(userdata,path);
    int fxn_ret = 0;
    int dfs_result;

    // Change file access and modification times.
    struct stat *statbuf = new struct stat;
    int sys_ret = stat(full_path, statbuf);

    if (sys_ret < 0) {
        fxn_ret = -errno;
        memset(statbuf, 0, sizeof(struct stat));
    }else{
        fxn_ret = sys_ret;
    }

    struct timespec new_atime = ts[0];
    struct timespec new_mtime = ts[1];

    struct timespec old_atime = statbuf->st_atim;
    struct timespec old_mtime = statbuf->st_mtim;

    if (old_atime.tv_sec != new_atime.tv_sec || old_atime.tv_nsec != new_atime.tv_nsec){
        // read operation
        flag = O_RDONLY;

        int utils_ret = 0;
        utils_ret = read_freshness_check(userdata, path);
        if (utils_ret < 0){
            return utils_ret;
        }
        sys_ret = utimensat(0, full_path, ts, flag);
    }

    if (old_mtime.tv_sec != new_mtime.tv_sec || old_mtime.tv_nsec != new_mtime.tv_nsec){
        // write operation
        flag = O_WRONLY;
        sys_ret = write_freshness_check(userdata, path);
        if (sys_ret < 0){
            return sys_ret;
        }
        sys_ret = utimensat(0, full_path, ts, flag);
        dfs_result = watdfs_cli_utimens_remote(userdata, path, ts);
        if (dfs_result < 0){
            return dfs_result;
        }
    }

    // Finally return the value we got from the server.
    return fxn_ret;
}
