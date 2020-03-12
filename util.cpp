//
// Starter code for CS 454
// You SHOULD change this file
//
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

#include<iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <map> // for recording the opened files
#include <time.h>

#include "rw_lock.h"
#include "watdfs_mutex.cpp"



/*
struct stat {
    dev_t     st_dev;         /* ID of device containing file
    ino_t     st_ino;         /* Inode number
    mode_t    st_mode;        /* File type and mode
    nlink_t   st_nlink;       /* Number of hard links *
    uid_t     st_uid;         /* User ID of owner *
    gid_t     st_gid;         /* Group ID of owner *
    dev_t     st_rdev;        /* Device ID (if special file) *
    off_t     st_size;        /* Total size, in bytes *
    blksize_t st_blksize;     /* Block size for filesystem I/O *
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated *

    /* Since Linux 2.6, the kernel supports nanosecond
     precision for the following timestamp fields.
     For the details before Linux 2.6, see NOTES. *

    struct timespec st_atim;  /* Time of last access *
    struct timespec st_mtim;  /* Time of last modification *
    struct timespec st_ctim;  /* Time of last status change *
};
*/

#define PRINT_ERR

int write_from_buffer_to_local_file(void *userdata, const char *path, const char *buf,
                              size_t size, off_t offset);
int write_from_local_file_to_buffer(void *userdata, const char *path, char *buf, size_t size,
                              off_t offset, struct fuse_file_info *fi);
int upload(void *userdata, const char *path, struct fuse_file_info *fi);
int download(void *userdata, const char *path, struct fuse_file_info *fi);

struct file_metadata{
    int fh_server_remote;
    int fh_client_local;

    struct timespec Tc; // last time checked freshness condition
};

struct global_state{
    // Global state client_persist_dir.
    char *client_persist_dir;
    time_t cache_interval;
    // Map: if a file is opened it should be in the map, if not delete it
    std::map<std::string, struct file_metadata *> opened_files; // local filename, file access code (on heap, don't forget to free !!!!!!!!)
};

//TODO
double timespec_diff(struct timespec T1, struct timespec T2){
    return (double) (difftime(T1.tv_sec, T2.tv_sec));
}
//TODO
struct timespec get_curr_time(){
    struct timespec *T_tmp_pointer = new struct timespec;
    (CLOCK_REALTIME, T_tmp_pointer);

    struct timespec T = *T_tmp_pointer;
    delete T_tmp_pointer;

    return T;
}

void set_validate_time(void * userdata, const char *path){
    ((global_state *) userdata)->opened_files[path]->Tc = get_curr_time();
}

char* get_full_path(void* userdata, const char *path) {
    int path_len = strlen(path);
    int dir_len = strlen(((global_state *) userdata)->client_persist_dir);
    int full_len = dir_len + path_len + 1;

    char *full_path = (char*)malloc(full_len);

    strcpy(full_path, ((global_state *) userdata)->client_persist_dir);
    strcat(full_path, path);

    return full_path;
}

//*************************** freshness check start ***************************
// if not fresh, update the file
int read_freshness_check(void *userdata, const char *path){// True if fresh
    int sys_ret, fxn_ret, dfs_ret;
    struct file_metadata * file_meta = ((global_state *) userdata)->opened_files[path];
    struct timespec Tc = file_meta->Tc;
    struct timespec T = get_curr_time();

    // get T_client and T_server
    struct stat* statbuf = new struct stat;
    char * full_path = get_full_path(userdata,path);

    sys_ret = stat(full_path, statbuf);
    if (sys_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }
    struct timespec T_client = statbuf->st_mtim;

    dfs_ret = watdfs_cli_getattr_remote(userdata, path, statbuf);
    if (dfs_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        dfs_ret = sys_ret;
    }
    struct timespec T_server = statbuf->st_mtim;

    if (!( timespec_diff(T, Tc ) < (double) ((global_state *) userdata)->cache_interval
        ||timespec_diff(T_client, T_server) == 0)){
        //std::cerr << "file timeout and freshed" << std::endl;
        struct fuse_file_info * fi = new struct fuse_file_info;
        fi->fh = file_meta->fh_server_remote;
        fi->flags = O_RDONLY;
        dfs_ret = download(userdata, path, fi);

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

int write_freshness_check(void *userdata, const char *path){// True if fresh
    int sys_ret, fxn_ret, dfs_ret;
    struct file_metadata * file_meta = ((global_state *) userdata)->opened_files[path];
    struct timespec Tc = file_meta->Tc;
    struct timespec T = get_curr_time();

    //std::cerr << "write_freshness_check" << std::endl;

    // get T_client and T_server
    struct stat* statbuf = new struct stat;
    char * full_path = get_full_path(userdata,path);

    sys_ret = stat(full_path, statbuf);
    if (sys_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }
    struct timespec T_client = statbuf->st_mtim;

    dfs_ret = watdfs_cli_getattr_remote(userdata, path, statbuf);
    if (dfs_ret < 0){
        delete statbuf;
        return sys_ret;
    }else{
        dfs_ret = sys_ret;
    }
    struct timespec T_server = statbuf->st_mtim;

    if (!( timespec_diff(T, Tc ) < (double) ((global_state *) userdata)->cache_interval
        ||timespec_diff(T_client, T_server) == 0)){

    if (!( timespec_diff(T, Tc ) < (double) ((global_state *) userdata)->cache_interval
          ||timespec_diff(T_client, T_server) == 0)){
        //std::cerr << "file timeout and freshed" << std::endl;
        struct fuse_file_info * fi = new struct fuse_file_info;
        fi->fh = file_meta->fh_server_remote;
        fi->flags = O_RDWR;
        dfs_ret = upload(userdata, path, fi); // T_server must be changed to T_client, in upload

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

//*************************** freshness check end ***************************

// true if file exists, false if file does not exist
bool check_if_file_exist_on_server(void *userdata, const char *path){
    int remote_ret = 0;
    struct stat * tmp_statbuf = new struct stat;
    remote_ret = watdfs_cli_getattr_remote(userdata, path, tmp_statbuf);
    return (remote_ret != -2);
}


bool file_in_cache(void* userdata, char *path){ // true in cache, false not in cache
    return (((global_state *)userdata)->opened_files.find(path) != ((global_state *)userdata)->opened_files.end());
}

bool file_opened(void* userdata, const char *path){// true opend, false not in cache or not opened
    if (((global_state *)userdata)->opened_files.find(path) == ((global_state *)userdata)->opened_files.end()){
        return false;
    }else{
        return true;
    }
}

// download from server to local path, the mtime will be synchronized for both client and the server
int download(void *userdata, const char *path, struct fuse_file_info *fi){
    //std::cerr << "download called" << std::endl;
    // initializations
    int dfs_result, sys_ret, fxn_ret;
    char * full_path = get_full_path(userdata,path);

    // Get file attributes from the server
    struct stat* statbuf = new struct stat;

    ////std::cerr << "remote getattr called in open" << std::endl;
    dfs_result = watdfs_cli_getattr_remote(userdata, path, statbuf);
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
        ((global_state *) userdata)->opened_files[path]->fh_client_local = local_fh;
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
    dfs_result = watdfs_cli_read_remote(userdata, path, buffer, size_of_file, 0, fi);
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
    ((global_state *) userdata)->opened_files[path]->fh_server_remote = fi->fh;// server
    ((global_state *) userdata)->opened_files[path]->fh_client_local = local_fh;// local

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

// the mtime will be synchronized for both client and the server
int upload(void *userdata, const char *path, struct fuse_file_info *fi){
    struct stat *statbuf = new struct stat;
    char *full_path = get_full_path(userdata, path);
    int sys_ret = stat(full_path, statbuf);
    int fxn_ret = 0;

    if (sys_ret < 0) {
        fxn_ret = -errno;
        memset(statbuf, 0, sizeof(struct stat));
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }

    off_t size_of_file = statbuf->st_size;
    char * buffer = (char *) malloc( size_of_file*sizeof(char));

    //truncate first
    // write from buffet to server
    std::cerr << "upload lock acquired" << std::endl;
    sys_ret = lock(path, RW_WRITE_LOCK);
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }

    sys_ret = watdfs_cli_truncate_remote(userdata, path, size_of_file);
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }
    // write from the file to buffer
    sys_ret = write_from_local_file_to_buffer(userdata, path, buffer, size_of_file, 0, fi);
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }
    std::cerr << "upload write remote fh" << fi->fh  << std::endl;
    sys_ret = watdfs_cli_write_remote(userdata, path, buffer, size_of_file, 0, fi);
    std::cerr << "upload write finished " << sys_ret << std::endl;
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }

    std::cerr << "upload unlock start" << std::endl;
    sys_ret = unlock(path, RW_WRITE_LOCK);
    std::cerr << "upload unlock finished" << std::endl;
    if (sys_ret < 0){
        free(buffer);
        delete statbuf;
        return sys_ret;
    }else{
        fxn_ret = sys_ret;
    }
    std::cerr << "upload lock released" << std::endl;

    //remote call utimens, set the modified time on server
    struct timespec * ts = new struct timespec [2];
    ts[0] = statbuf->st_atim;
    ts[1] = statbuf->st_mtim;

    int dfs_result = watdfs_cli_utimens_remote(userdata, path, ts);
    if (dfs_result < 0){
        fxn_ret = dfs_result;
        delete statbuf;
        free(buffer);
        free(full_path);
        return fxn_ret;
    }else{
        fxn_ret = dfs_result;
    }

    delete statbuf;
    free(buffer);
    free(full_path);
    return fxn_ret;
}

int write_from_local_file_to_buffer(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi){
    //std::cerr <<"write_from_local_file_to_buffer called fh " <<  ((global_state*)userdata)->opened_files[path]->fh_client_local << std::endl;
    // Remember that size may be greater then the maximum array size of the RPC
    int fxn_ret = 0;
    int sys_ret = 0;
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
        int fh = ((global_state*)userdata)->opened_files[path]->fh_client_local;
        //std::cout << fh << " " << size_current_read << " " << offset << std::endl;
        sys_ret = pread(((global_state*)userdata)->opened_files[path]->fh_client_local, buf_tmp, size_current_read, offset);

        if (sys_ret < 0){
            fxn_ret = -errno;
            break;
        }else{// read file successful
            memcpy(buf+actually_read, buf_tmp, sys_ret);
            ////std::cerr <<"line been read "<< buf << std::endl;
            actually_read = actually_read + sys_ret;
            fxn_ret = size;
            if (sys_ret < size_current_read){//EOF meet
                break;
            }
        }
        size_dynamic = size_dynamic - size_current_read;
        offset = offset + size_current_read;
    }
    ////std::cerr << buf << std::endl;
    //std::cerr <<"actually read size "<< actually_read << std::endl;
    free(buf_tmp);

    // return number of bytes actually read
    return fxn_ret;
}

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

        sys_ret = pwrite(((global_state*)userdata)->opened_files[path]->fh_client_local, buf, size_current_write, offset);

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
