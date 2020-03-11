//
// Starter code for CS 454/654
// You SHOULD change this file
//

#include "watdfs_client.h"

#include "rpc.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <iostream>
#include <map>
#include <string>

#define READ_LOCK 0
#define WRITE_LOCK 1

#define RETURN_IF_NEG(r)                                                       \
    do {                                                                       \
        if (r < 0) {                                                           \
            return r;                                                          \
        }                                                                      \
        break;                                                                 \
    } while (0)

#define EINVAL_IF_ERR(r)                                                       \
    do {                                                                       \
        if (r != 0) {                                                          \
            return -EINVAL;                                                    \
        }                                                                      \
        break;                                                                 \
    } while (0)



// states
char *CACHE_DIR = nullptr;
time_t CACHE_INTERVAL;
// int volatile download_status = 0;// -2 means server file not found




// ----------------utils----------------
static void severe_log_ifneg(int r, const char *str) {// "should never happen" bug
    if (r < 0) {
# ifdef PRINT_ERR
        std::cerr << "[!!] " << str << std::endl;
#endif
    }
}

static char *get_cache_path(const char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len = strlen(CACHE_DIR);
    int full_len = dir_len + short_path_len + 1;
    char *full_path = (char *)malloc(full_len);
    strcpy(full_path, CACHE_DIR);
    strcat(full_path, short_path);
    return full_path;
}

static void LOGGER(const char *str) {
# ifdef PRINT_ERR
    std::cerr << str << std::endl;
#endif
}
static void LOGGER(const char *str, int i) {
# ifdef PRINT_ERR
    std::cerr << str << i << std::endl;
#endif
}
static void LOGGER(const char *str1, const char *str2) {
# ifdef PRINT_ERR
    std::cerr << str1 << str2 << std::endl;
#endif
}

// ----------------utils----------------


// ----------------file_modes_t----------------

struct file_info {
    int mode;
    uint64_t fd_c;
    time_t tc;
};

typedef std::map<std::string, file_info> file_modes_t;

static int file_modes_init(file_modes_t *m) {
    // ...
    return 0;
}
static int file_modes_has(file_modes_t *m, const char *path) {
    return m->count(std::string(path)) > 0;
}
static int file_modes_getmode(file_modes_t *m, const char *path) {
    if (file_modes_has(m, path)) {
        file_info info = (*m)[std::string(path)];
        return info.mode == -1 ? -1 : (info.mode & O_ACCMODE);
    }
    return -1;
}
static uint64_t file_modes_getfdc(file_modes_t *m, const char *path) {
    if (file_modes_has(m, path)) {
        file_info info = (*m)[std::string(path)];
        return info.fd_c;
    }
    return -1;
}
static time_t file_modes_gettc(file_modes_t *m, const char *path) {
    if (file_modes_has(m, path)) {
        file_info info = (*m)[std::string(path)];
        return info.tc;
    }
    return -1;
}
static int file_modes_put(file_modes_t *m, const char *path, int mode, uint64_t fd_c, time_t tc) {
    file_info new_info = { mode, fd_c, tc };
    if (file_modes_has(m, path)) {
        (*m)[std::string(path)] = new_info;
    } else {
        m->insert(std::make_pair(std::string(path),new_info));
    }
    return 0;
}
static int file_modes_destroy(file_modes_t *m) {
    // ...
    return 0;
}

static void set_tc_to_now(file_modes_t *m, const char *path) {
    if (!file_modes_has(m, path)) {
        severe_log_ifneg(-1, (char*)"set_tc_to_now: no such path in map");
    }
    (*m)[std::string(path)].tc = time(NULL);
}

// ----------------file_modes_t----------------


// ----------------lock/unlock----------------
static void LOCK(const char *path, int mode) {
    int retcode;
    int argTypes[4];
    void **args = (void **)malloc(3 * sizeof(void *));
    int pathlen = strlen(path) + 1;
    argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    argTypes[3] = 0;
    args[0] = (void *)path;
    args[1] = (void *)&mode;
    args[2] = (void *)&retcode;
    rpcCall((char *)"lock", argTypes, args);
    severe_log_ifneg(retcode, (char *)"LOCK: rpc lock retcode < 0");
    free(args);
}
static void UNLOCK(const char *path, int mode) {
    int retcode;
    int argTypes[4];
    void **args = (void **)malloc(3 * sizeof(void *));
    int pathlen = strlen(path) + 1;
    argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    argTypes[3] = 0;
    args[0] = (void *)path;
    args[1] = (void *)&mode;
    args[2] = (void *)&retcode;
    rpcCall((char *)"unlock", argTypes, args);
    severe_log_ifneg(retcode, (char *)"UNLOCK: rpc unlock retcode < 0");
    free(args);
}
// ----------------lock/unlock----------------


// ----------------rpc helpers, local call helpers----------------
static int truncate_rem(const char *path, off_t newsize) {
    LOGGER((char*)"REMOTE truncate_rem called with path: ",path);
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    args[1] = (void *)&newsize;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;
    int rpc_ret = rpcCall((char *)"truncate", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE truncate_rem: retcode=",retcode);
    return retcode;
}

static int truncate_(const char *cache_path, off_t newsize) {
    LOGGER((char*)"LOCAL truncate_: cache_path=",cache_path);
    int sys_ret;
    sys_ret = truncate(cache_path, newsize);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL truncate_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int getattr_rem(const char *path, struct stat *statbuf) {
    LOGGER((char*)"REMOTE getattr_rem called with path: ",path);
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) |
                   sizeof(struct stat);
    args[1] = (void *)statbuf;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;

    int rpc_ret = rpcCall((char *)"getattr", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    LOGGER((char*)"REMOTE getattr_rem: retcode=",retcode);
    if (retcode < 0) {
        memset(statbuf, 0, sizeof(struct stat));
    }
    free(args);
    return retcode;
}

static int getattr_(const char *cache_path, struct stat *statbuf) {
    LOGGER((char*)"LOCAL getattr_: cache_path=",cache_path);
    int sys_ret;
    sys_ret = stat(cache_path, statbuf);
    if (sys_ret < 0) {
        memset(statbuf, 0, sizeof(struct stat));
        LOGGER((char*)"LOCAL getattr_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int fgetattr_rem(const char *path, struct stat *statbuf, struct fuse_file_info *fi) {
    int ARG_COUNT = 4;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) |
                   sizeof(struct stat); // statbuf
    args[1] = (void *)statbuf;
    argTypes[2] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[2] = (void *)fi;
    argTypes[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[3] = (void *)&retcode;
    argTypes[4] = 0;

    int rpc_ret = rpcCall((char *)"fgetattr", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    LOGGER((char*)"REMOTE fgetattr_rem: retcode=",retcode);
    if (retcode < 0) {
        memset(statbuf, 0, sizeof(struct stat));
    }
    free(args);
    return retcode;
}

static int fgetattr_(const char *cache_path, struct stat *statbuf, struct fuse_file_info *fi) {
    LOGGER((char*)"LOCAL fgetattr_: cache_path=",cache_path);
    int sys_ret;
    sys_ret = fstat(fi->fh, statbuf);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL fgetattr_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int mknod_rem(const char *path, mode_t mode, dev_t dev) {
    int ARG_COUNT = 4;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
    args[1] = (void *)&mode;
    argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    args[2] = (void *)&dev;
    argTypes[3] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[3] = (void *)&retcode;
    argTypes[4] = 0;

    int rpc_ret = rpcCall((char *)"mknod", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    LOGGER((char*)"REMOTE mknod_rem: retcode=",retcode);
    free(args);
    return retcode;
}

static int mknod_(const char *cache_path, mode_t mode, dev_t dev) {
    LOGGER((char*)"LOCAL mknod_: cache_path=",cache_path);
    int sys_ret;
    sys_ret = mknod(cache_path, mode, dev);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL mknod_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int open_rem(const char *path, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE open_rem called with path: ",path);
    LOGGER((char*)"REMOTE open_rem: flags:",(fi->flags));
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void *)fi;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;
    int rpc_ret = rpcCall((char *)"open", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    LOGGER((char*)"REMOTE open_rem: retcode=",retcode);
    LOGGER((char*)"REMOTE open_rem: fd<=",fi->fh);
    free(args);
    return retcode;
}

static int superopen_rem(const char *path, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE superopen_rem called with path: ",path);
    LOGGER((char*)"REMOTE superopen_rem: flags:",(fi->flags));
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void *)fi;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;
    int rpc_ret = rpcCall((char *)"superopen", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    LOGGER((char*)"REMOTE superopen_rem: retcode=",retcode);
    LOGGER((char*)"REMOTE superopen_rem: fd<=",fi->fh);
    free(args);
    return retcode;
}

static int open_(const char *cache_path, int flags, file_modes_t *file_modes) {
    LOGGER((char*)"LOCAL open_: cache_path=",cache_path);
    int sys_ret;
    if (flags == O_CREAT) {
        LOGGER((char*)"LOCAL open_: O_CREAT type open");
        sys_ret = open(cache_path, O_CREAT | O_WRONLY, S_IRWXU);
    } else {
        LOGGER((char*)"LOCAL open_: regular type open");
        sys_ret = open(cache_path, flags);
    }

    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL open_: errno=",errno);
        return -errno;
    }
    file_modes_put(file_modes, cache_path, flags, sys_ret, -1);
    LOGGER((char*)"LOCAL open_:      put: client_fd=",sys_ret);
    LOGGER((char*)"LOCAL open_: success: fd<=",sys_ret);
    return sys_ret;// sys_ret is the fd
}

static int open_sys(const char *path, int flags) {
    int sys_ret;
    sys_ret = open(path, flags | O_CREAT, S_IRWXU);
    if (sys_ret < 0) {
        LOGGER((char*)"  open_sys: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int read_rem(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE read_rem called with path: ",path);
    LOGGER((char*)"REMOTE read_rem: size=",size);
    LOGGER((char*)"REMOTE read_rem: offset=",offset);
    int ARG_COUNT = 6;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    argTypes[3] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    args[3] = (void *)&offset;
    argTypes[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[4] = (void *)fi;
    argTypes[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[5] = (void *)&retcode;
    argTypes[6] = 0;

    size_t sizeRemain = size;
    while (sizeRemain > 0) {
        size_t segment = sizeRemain > MAX_ARRAY_LEN ? MAX_ARRAY_LEN : sizeRemain;
        argTypes[1] = (1 << ARG_OUTPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | segment;
        args[1] = (void *)buf;
        args[2] = (void *)&segment;
        int rpc_ret = rpcCall((char *)"read", argTypes, args);
        EINVAL_IF_ERR(rpc_ret);
        if (retcode < 0) {
            return retcode;
        }
        sizeRemain -= (size_t)retcode;
        buf += retcode;
        if (retcode < segment) break;
    }
    LOGGER((char*)"REMOTE read_rem: ret size=",size-sizeRemain);
    return size - sizeRemain;
}

/*
static int read_(const char *cache_path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"LOCAL read_ called with path: ",cache_path);
    LOGGER((char*)"LOCAL read_: size=",size);
    LOGGER((char*)"LOCAL read_: offset=",offset);
    size_t sizeRemain = size;
    while (sizeRemain > 0) {
        size_t segment = sizeRemain > MAX_ARRAY_LEN ? MAX_ARRAY_LEN : sizeRemain;
        int sys_ret = pread(fi->fh, buf, size, offset);
        if (sys_ret < 0) {
            LOGGER((char*)"server: read: errno=",errno);
            return -errno;
        }
        sizeRemain -= (size_t)sys_ret;
        buf += sys_ret;
        if (sys_ret < segment) break;
    }
    LOGGER((char*)"LOCAL read_: ret size=",size-sizeRemain);
    return size-sizeRemain;
}
*/
static int read_(const char *cache_path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"LOCAL read_ called with path: ",cache_path);
    LOGGER((char*)"LOCAL read_: fd=",fi->fh);
    LOGGER((char*)"LOCAL read_: size=",size);
    LOGGER((char*)"LOCAL read_: offset=",offset);
    int sys_ret;
    sys_ret = pread(fi->fh, buf, size, offset);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL read_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int write_rem(const char *path, const char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE write_rem called with path: ",path);
    LOGGER((char*)"REMOTE write_rem: size=",size);
    LOGGER((char*)"REMOTE write_rem: offset=",offset);
    int ARG_COUNT = 6;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[2] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    argTypes[3] = (1 << ARG_INPUT) | (ARG_LONG << 16);
    args[3] = (void *)&offset;
    argTypes[4] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[4] = (void *)fi;
    argTypes[5] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[5] = (void *)&retcode;
    argTypes[6] = 0;

    size_t sizeRemain = size;
    while (sizeRemain > 0) {
        size_t segment = sizeRemain > MAX_ARRAY_LEN ? MAX_ARRAY_LEN : sizeRemain;
        argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | segment;
        args[1] = (void *)buf;
        args[2] = (void *)&segment;
        int rpc_ret = rpcCall((char *)"write", argTypes, args);
        EINVAL_IF_ERR(rpc_ret);
        if (retcode < 0) {
            return retcode;
        }
        sizeRemain -= (size_t)retcode;
        buf += retcode;
    }
    LOGGER((char*)"REMOTE write_rem: ret size=",size);
    return size;
}

static int write_(const char *cache_path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"LOCAL write_ called with path: ",cache_path);
    LOGGER((char*)"LOCAL write_: size=",size);
    LOGGER((char*)"LOCAL write_: offset=",offset);
    int sys_ret;
    sys_ret = pwrite(fi->fh, buf, size, offset);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL write_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

/*
static int utimens_rem(const char *path, const struct timespec ts[2]) {
    LOGGER((char*)"REMOTE utimens_rem called with path:",path);
    LOGGER((char*)"REMOTE utimens_rem: ts[0]: ",ts[0].tv_sec);
    LOGGER((char*)"REMOTE utimens_rem: ts[1]: ",ts[1].tv_sec);
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | 2*sizeof(struct timespec);
    args[1] = (void *)ts;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;
    int rpc_ret = rpcCall((char *)"utimens", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE utimens_rem: retcode=",retcode);
    return retcode;
}
*/
static int utimens_rem(const char *path, time_t atime, time_t mtime) {
    LOGGER((char*)"REMOTE utimens_rem called with path: ",path);
    struct utimbuf times;
    times.actime = atime;
    times.modtime = mtime;

    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct utimbuf);
    args[1] = (void *)&times;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;

    int rpc_ret = rpcCall((char *)"utime", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE utimens_rem: retcode=",retcode);
    return retcode;
}

static int utimens_(const char *cache_path, time_t atime, time_t mtime) {
    LOGGER((char*)"LOCAL utimens_ called with path: ",cache_path);
    int sys_ret;
    struct utimbuf times;
    times.actime = atime;
    times.modtime = mtime;
    sys_ret = utime(cache_path, &times);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL utimens_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int release_rem(const char *path, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE release_rem called with fd=",fi->fh);
    LOGGER((char*)"REMOTE release_rem: path: ",path);
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void *)fi;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;

    int rpc_ret = rpcCall((char *)"release", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE release_rem: retcode=",retcode);
    return retcode;
}

static int superrelease_rem(const char *path, struct fuse_file_info *fi) {
    LOGGER((char*)"REMOTE superrelease_rem called with fd=",fi->fh);
    LOGGER((char*)"REMOTE superrelease_rem: path: ",path);
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void *)fi;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;

    int rpc_ret = rpcCall((char *)"superrelease", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE superrelease_rem: retcode=",retcode);
    return retcode;
}

static int release_(const char *cache_path, uint64_t fd, file_modes_t *file_modes) {
    LOGGER((char*)"LOCAL release_ called with fd=",fd);
    LOGGER((char*)"LOCAL release_ path: ",cache_path);

    // mark as close in map
    file_modes_put(file_modes, cache_path, -1, -1, -1);

    int sys_ret;
    sys_ret = close(fd);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL release_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int release_sys(const char *path, int fd) {
    int sys_ret;
    sys_ret = close(fd);
    if (sys_ret < 0) {
        LOGGER((char*)"  release_sys: errno=",errno);
        return -errno;
    }
    return sys_ret;
}

static int fsync_rem(const char *path, struct fuse_file_info *fi) {
    int ARG_COUNT = 3;
    void **args = (void **)malloc(ARG_COUNT * sizeof(void *));
    int argTypes[ARG_COUNT + 1];
    int pathlen = strlen(path) + 1;
    argTypes[0] =
        (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | pathlen;
    args[0] = (void *)path;
    argTypes[1] = (1 << ARG_INPUT) | (1 << ARG_ARRAY) | (ARG_CHAR << 16) | sizeof(struct fuse_file_info);
    args[1] = (void *)fi;
    argTypes[2] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
    int retcode;
    args[2] = (void *)&retcode;
    argTypes[3] = 0;
    int rpc_ret = rpcCall((char *)"fsync", argTypes, args);
    EINVAL_IF_ERR(rpc_ret);
    free(args);
    LOGGER((char*)"REMOTE fsync_rem: retcode=",retcode);
    return retcode;
}

static int fsync_(const char *cache_path, int fd) {
    LOGGER((char*)"LOCAL fsync_ called with fd=",fd);
    LOGGER((char*)"LOCAL fsync_ path: ",cache_path);
    int sys_ret;
    sys_ret = fsync(fd);
    if (sys_ret < 0) {
        LOGGER((char*)"LOCAL release_: errno=",errno);
        return -errno;
    }
    return sys_ret;
}
// ----------------rpc helpers, local call helpers----------------


// freshness check
static bool freshness_check(file_modes_t *file_modes, const char *cache_path, const char *path) {
    LOGGER((char*)"  -FRESHNESS_CHECK begin: cache_path: ",cache_path);
    int retcode;
    bool local = false;
    struct stat stat_client;
    retcode = getattr_(cache_path, &stat_client);
    RETURN_IF_NEG(retcode);
    time_t Tclient = stat_client.st_mtime;
    time_t Tc = file_modes_gettc(file_modes, cache_path);
    severe_log_ifneg(Tc, (char *)"freshness_check: can't get Tc");
    time_t T = time(NULL);
    bool cond1 = difftime(CACHE_INTERVAL, difftime(T,Tc)) > 0;
    if (cond1) {
        local = true;
    } else {// need to consult the server
        struct stat stat_server;
        retcode = getattr_rem(path, &stat_server);
        RETURN_IF_NEG(retcode);
        time_t Tserver = stat_server.st_mtime;
        bool cond2 = difftime(Tclient, Tserver) == 0;
        if (cond2) local = true;
    }
    LOGGER((char*)"  -FRESHNESS_CHECK end: ",local? (char*)"LOCAL":(char*)"NOT LOCAL");
    return local;
}

// download
// need to check existnece before calling download
static int download(void *userdata, const char *path) { // server -> client
    LOCK(path, READ_LOCK);
    LOGGER((char*)"---DOWNLOAD begin");
    // download_status = 0;
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode;

    // get file attributes from the server
    struct stat stat_server;
    retcode = getattr_rem(path, &stat_server);
    // download_status = retcode;
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: success (server file DNE)");
        UNLOCK(path, READ_LOCK);
        return 0;
    }

    time_t server_last_m = stat_server.st_mtime;
    // time_t server_last_a = stat_server.st_atime;
    size_t size = stat_server.st_size;
    // LOGGER((char*)"UPLOAD: curr time=",time(NULL));
    // LOGGER((char*)"UPLOAD: server_last_a=",server_last_a);
    // LOGGER((char*)"UPLOAD: server_last_m=",server_last_m);
    LOGGER((char*)"DOWNLOAD: client size=",size);

    // "force open" at client
    struct stat stat_client;
    retcode = getattr_(cache_path, &stat_client);
    LOGGER((char*)"DOWNLOAD existence check: retcode=",retcode);
    int fd_client;
    if (retcode < 0) {// no client file
        LOGGER((char*)"DOWNLOAD: start force open at client !");
        // open using create mode
        retcode = open_sys(cache_path, O_CREAT);
        if (retcode < 0) {
            LOGGER((char*)"---DOWNLOAD end: fail at Client open_sys(O_CREAT):",retcode);
            UNLOCK(path, READ_LOCK);
            return retcode;
        }
        fd_client = retcode;
    } else {// client file exists
        // open using write mode
        retcode = open_sys(cache_path, O_WRONLY);
        if (retcode < 0) {
            LOGGER((char*)"---DOWNLOAD end: fail at Client open_sys(O_WRONLY):",retcode);
            UNLOCK(path, READ_LOCK);
            return retcode;
        }
        fd_client = retcode;
    }
    struct fuse_file_info fi_client;
    fi_client.fh = fd_client;

    // open the file from server in read only mode
    struct fuse_file_info fi_server;
    fi_server.flags = O_RDONLY;
    // retcode = open_rem(path, &fi_server);
    retcode = superopen_rem(path, &fi_server);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at open_rem:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    // need to truncate the file at the client
    retcode = truncate_(cache_path, (off_t)size);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at truncate_:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    // read the file from the server
    char *buf = (char *)malloc((size+1)*sizeof(char));
    retcode = read_rem(path, buf, size, 0, &fi_server);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at read_rem:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }
    // close the file at server
    // retcode = release_rem(path, &fi_server);
    retcode = superrelease_rem(path, &fi_server);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at release_rem:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }
    // write the file to the client
    retcode = write_(cache_path, buf, size, 0, &fi_client);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at write_:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }
    // close the file at client
    retcode = release_sys(cache_path, fd_client);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at release_sys:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    // update the file metadata at the client
    //   - update T_client to be equal to T_server
    //   - update Tc to the current time
    retcode = utimens_(cache_path, server_last_m, server_last_m);
    if (retcode < 0) {
        LOGGER((char*)"---DOWNLOAD end: fail at utimens_:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }
    // time_t curr_time = time(NULL);
    // file_modes_put(file_modes, cache_path, -1, -1, curr_time);

    LOGGER((char*)"---DOWNLOAD end: success");
    UNLOCK(path, READ_LOCK);
    return 0;
}

// upload
static int upload(void *userdata, const char *path) { // client -> server
    LOCK(path, WRITE_LOCK);
    LOGGER((char*)"---UPLOAD begin");
    LOGGER((char*)"UPLOAD: path: ",path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode;

    // "force open" at server in write mode
    // int fd_server;
    struct fuse_file_info fi_server;
    /*
    if (download_status == -2) {// no server file
        LOGGER((char*)"UPLOAD: start force open at server !");
        download_status = 0;
        // open using create mode
        fi_server.flags = O_CREAT;
        // retcode = open_rem(path, &fi_server);
        retcode = superopen_rem(path, &fi_server);
        if (retcode < 0) {
            LOGGER((char*)"---UPLOAD end: fail at Server open_rem(O_CREAT):",retcode);
            UNLOCK(path, WRITE_LOCK);
            return retcode;
        }
        // fd_server = retcode;
    } else {// server file exists
        download_status = 0;
    */

        // open using write mode
        fi_server.flags = O_WRONLY;
        // retcode = open_rem(path, &fi_server);
        retcode = superopen_rem(path, &fi_server);
        if (retcode < 0) {
            LOGGER((char*)"---UPLOAD end: fail at Server open_rem(O_WRONLY):",retcode);
            UNLOCK(path, WRITE_LOCK);
            return retcode;
        }
        // fd_server = retcode;
    /*}*/

    // get file attributes from the client
    struct stat stat_client;
    retcode = getattr_(cache_path, &stat_client);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at getattr_:",retcode);
        UNLOCK(path, WRITE_LOCK);
        return retcode;
    }
    time_t client_last_m = stat_client.st_mtime;
    // time_t client_last_a = stat_client->st_atime;
    size_t size = stat_client.st_size;
    // LOGGER((char*)"UPLOAD: curr time=",time(NULL));
    // LOGGER((char*)"UPLOAD: client_last_a=",client_last_a);
    // LOGGER((char*)"UPLOAD: client_last_m=",client_last_m);
    LOGGER((char*)"UPLOAD: client size=",size);

    // open the file at client in read only mode
    retcode = open_sys(cache_path, O_RDONLY);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at open_sys:",retcode);
        UNLOCK(path, WRITE_LOCK);
        return retcode;
    }
    int fd_client = retcode;

    // need to truncate the file at the server
    retcode = truncate_rem(path, (off_t)size);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at truncate_rem:",retcode);
        UNLOCK(path, WRITE_LOCK);
        return retcode;
    }

    // read the file at client ???
    char *buf = (char *)malloc((size+1)*sizeof(char));
    struct fuse_file_info fi_client;
    fi_client.fh = fd_client;
    retcode = read_(cache_path, buf, size, 0, &fi_client);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at read_:",retcode);
        UNLOCK(path, WRITE_LOCK);
        return retcode;
    }

    // close the file at client
    retcode = release_sys(cache_path, fd_client);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at release_sys:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    // write the file to the server ???
    retcode = write_rem(path, buf, size, 0, &fi_server);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at write_rem:",retcode);
        UNLOCK(path, WRITE_LOCK);
        return retcode;
    }

    // close the file at server
    // retcode = release_rem(path, &fi_server);
    retcode = superrelease_rem(path, &fi_server);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at release_rem:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    // update T_server to T_client
    /*
    long nsec = (long)client_last_a*1000000000;
    struct timespec atime = { client_last_a, nsec };
    nsec = (long)client_last_m*1000000000;
    struct timespec mtime = { client_last_m ,nsec };
    struct timespec ts[2] = { atime, mtime };
    retcode = utimens_rem(path, ts);
    */
    retcode = utimens_rem(path, client_last_m, client_last_m);
    if (retcode < 0) {
        LOGGER((char*)"---UPLOAD end: fail at utimens_rem:",retcode);
        UNLOCK(path, READ_LOCK);
        return retcode;
    }

    LOGGER((char*)"---UPLOAD end: success");
    UNLOCK(path, WRITE_LOCK);
    return 0;
}

static int fetch_if_needed(void *userdata, const char *cache_path, const char *path, bool write_call, int mode) {
    file_modes_t *file_modes = (file_modes_t *)userdata;
    bool should_fetch;
    if (mode < 0) {
        should_fetch = true;
    } else if (write_call && mode == O_RDONLY) {
        return -EMFILE;
    } else if (write_call && mode != O_RDONLY) {
        should_fetch = false;// not fetch for now in write calls
    } else if (!write_call && mode == O_RDONLY) {
        bool local = freshness_check(file_modes, cache_path, path);
        if (local) should_fetch = false;
        else should_fetch = true;

        // update freshness
        struct stat stat_server;
        getattr_rem(path, &stat_server);
        time_t server_last_m = stat_server.st_mtime;
        utimens_(cache_path, server_last_m, server_last_m);
        set_tc_to_now(file_modes, cache_path);

    } else {
        should_fetch = false;
    }

    if (should_fetch) {
        download(userdata, path);
    }
    return 0;
}


// watdfs_cli_*:
// SETUP AND TEARDOWN
// void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache, time_t cache_interval, int *ret_code) {
//
//     // temp loggers:
//     LOGGER("O_ACCMODE=",O_ACCMODE);
//     LOGGER("O_RDONLY=",O_RDONLY);
//     LOGGER("O_WRONLY=",O_WRONLY);
//     LOGGER("O_RDWR=",O_RDWR);
//
//
//     // TODO: set `ret_code` to 0 if everything above succeeded else some appropriate
//     // non-zero value.
//     *ret_code = 0;
//
//     // TODO: set up the RPC library by calling `rpcClientInit`.
//     int ret = rpcClientInit();
//
//     if (ret < 0) {
//     # ifdef PRINT_ERR
//         std::cerr << "rpcClientInit failed: ret=" << ret << std::endl;
//     #endif
//     }
//
//     // TODO: check the return code of the `rpcClientInit` call
//     // `rpcClientInit` may fail, for example, if an incorrect port was exported.
//     *ret_code = ret;
//
//     // It may be useful to print to stderr or stdout during debugging.
//     // Important: Make sure you turn off logging prior to submission!
//     // One useful technique is to use pre-processor flags like:
//     // # ifdef PRINT_ERR
//     // std::cerr << "Failed to initialize RPC Client" << std::endl;
//     // #endif
//     // Tip: Try using a macro for the above to minimize the debugging code.
//
//     // TODO Initialize any global state that you require for the assignment and return it.
//     // The value that you return here will be passed as userdata in other functions.
//     // In A1, you might not need it, so you can return `nullptr`.
//
//     file_modes_t *m = new std::map<std::string, file_info>();
//     void *userdata = (void *)m;
//     LOGGER((char*)"watdfs_cli_init: map init done.");
//
//     // TODO: save `path_to_cache` and `cache_interval` (for A3).
//     CACHE_DIR = (char *)malloc(strlen(path_to_cache)+1);
//     strcpy(CACHE_DIR, path_to_cache);
//     CACHE_INTERVAL = cache_interval;
//
//     // Return pointer to global state data.
//     return userdata;
// }

void watdfs_cli_destroy(void *userdata) {
    // TODO: clean up your userdata state.
    file_modes_destroy((file_modes_t *)userdata);

    // TODO: tear down the RPC library by calling `rpcClientDestroy`.
    rpcClientDestroy();
}


// GET FILE ATTRIBUTES
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) {
    LOGGER((char*)"------CLI_getattr called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    /*
    // check existence // .
    struct stat statbuf_tmp;
    retcode = getattr_rem(path, &statbuf_tmp);
    RETURN_IF_NEG(retcode);
    */
    // check open state
    int mode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_getattr: open state=",mode);
    retcode = fetch_if_needed(userdata, cache_path, path, false, mode);
    RETURN_IF_NEG(retcode);

    // perform locally
    fxn_ret = getattr_(cache_path, statbuf);

    free(cache_path);
    LOGGER((char*)"------CLI_getattr end: success: ",fxn_ret);
    return fxn_ret;
}

int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf, struct fuse_file_info *fi) {
    LOGGER((char*)"------CLI_fgetattr called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    // check open state
    int mode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_fgetattr: open state=",mode);
    retcode = fetch_if_needed(userdata, cache_path, path, false, mode);
    RETURN_IF_NEG(retcode);

    fxn_ret = fgetattr_(cache_path, statbuf, fi);

    free(cache_path);
    LOGGER((char*)"------CLI_fgetattr success: ",fxn_ret);
    return fxn_ret;
}

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) {// Called to create a file.
    LOGGER((char*)"------CLI_mknod called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    /*
    // check existence // .
    struct stat statbuf;
    retcode = getattr_rem(path, &statbuf);
    if (retcode == 0) {
        free(cache_path);
        return -EEXIST;
    }
    */

    // check open state
    int fmode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_mknod: open state=",fmode);
    retcode = fetch_if_needed(userdata, cache_path, path, true, fmode);
    RETURN_IF_NEG(retcode);

    // mknod locally
    retcode = mknod_(cache_path, mode, dev);

    // mknod at server
    fxn_ret = mknod_rem(path, mode, dev);

    // synchronously write client's copy of the file to server
    // retcode = upload(userdata, path);

    free(cache_path);
    LOGGER((char*)"------CLI_mknod end: success");
    return fxn_ret;
}

int watdfs_cli_open(void *userdata, const char *path, struct fuse_file_info *fi) {
    LOGGER((char*)"------CLI_open called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    // ensure to not open twice
    int file_mode = file_modes_getmode(file_modes, cache_path);
    if (file_mode >= 0) {
        LOGGER((char*)"CLI_open: don't open twice(-EMFILE): file_mode=",file_mode);
        return -EMFILE;
    }

    // pull from server
    retcode = download(userdata, path);

    // the actual open at server
    LOGGER((char*)"CLI_open:      actual open at server begins");
    retcode = open_rem(path, fi);
    RETURN_IF_NEG(retcode);
    fxn_ret = retcode;
    LOGGER((char*)"CLI_open:      actual open at server done");

    // the actual open at client
    LOGGER((char*)"CLI_open:      actual open at client begins");
    retcode = open_(cache_path, fi->flags, file_modes);
    RETURN_IF_NEG(retcode);
    LOGGER((char*)"CLI_open:      actual open at client done");

    // track open files
    // int open_mode = fi->flags & O_ACCMODE;
    // file_modes_put(file_modes, cache_path, open_mode, client_fd, server_fd, 0);

    free(cache_path);
    LOGGER((char*)"------CLI_open end: success: ", fxn_ret);
    return fxn_ret;
}

int watdfs_cli_release(void *userdata, const char *path, struct fuse_file_info *fi) {// Called during close, but possibly asynchronously.
    LOGGER((char*)"------CLI_release called: ", path);
    LOGGER((char*)"CLI_release: given fd=",fi->fh);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    uint64_t fdc = file_modes_getfdc(file_modes, cache_path);
    LOGGER((char*)"CLI_release:      get: fdc=",fdc);

    // close at both client and server
    retcode = release_(cache_path, fdc, file_modes);
    RETURN_IF_NEG(retcode);
    retcode = release_rem(path, fi);
    RETURN_IF_NEG(retcode);
    fxn_ret = retcode;

    // upload to server before closing
    retcode = upload(userdata, path);
    RETURN_IF_NEG(retcode);

    free(cache_path);
    LOGGER((char*)"------CLI_release end: success");
    return fxn_ret;
}

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"------CLI_read called: ", path);
    LOGGER((char*)"CLI_read: fd=", fi->fh);
    LOGGER((char*)"CLI_read: size=", (int)size);
    LOGGER((char*)"CLI_read: offset=", (int)offset);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    // check open state
    int fmode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_read: open state=",fmode);
    retcode = fetch_if_needed(userdata, cache_path, path, false, fmode);
    RETURN_IF_NEG(retcode);

    int client_fd = file_modes_getfdc(file_modes, cache_path);
    severe_log_ifneg(client_fd, (char*)"CLI_read: can't get client_fd from map");
    struct fuse_file_info client_fi;
    client_fi.fh = client_fd;
    fxn_ret = read_(cache_path, buf, size, offset, &client_fi);
    RETURN_IF_NEG(fxn_ret);

    free(cache_path);
    LOGGER((char*)"------CLI_read end: success: ",fxn_ret);
    return fxn_ret;
}

int watdfs_cli_write(void *userdata, const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    LOGGER((char*)"------CLI_write called: ", path);
    LOGGER((char*)"CLI_write: size=", (int)size);
    LOGGER((char*)"CLI_write: offset=", (int)offset);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    // check open state
    int fmode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_write: open state=",fmode);
    retcode = fetch_if_needed(userdata, cache_path, path, true, fmode);
    RETURN_IF_NEG(retcode);

    int client_fd = file_modes_getfdc(file_modes, cache_path);
    severe_log_ifneg(client_fd, (char*)"CLI_write: can't get client_fd from map");
    struct fuse_file_info client_fi;
    client_fi.fh = client_fd;
    fxn_ret = write_(cache_path, buf, size, offset, &client_fi);
    RETURN_IF_NEG(fxn_ret);

    // freshness check
    bool local = freshness_check(file_modes, cache_path, path);
    if (local) {
        LOGGER((char*)"------CLI_write end: success: ",fxn_ret);
        free(cache_path);
        return fxn_ret;
    }

    // synchronously write client's copy of the file to server
    retcode = upload(userdata, path);
    set_tc_to_now(file_modes, cache_path);
    RETURN_IF_NEG(retcode);

    free(cache_path);
    LOGGER((char*)"------CLI_write end: success: ",fxn_ret);
    return fxn_ret;
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) {// Change the file size to newsize.
    LOGGER((char*)"------CLI_truncate called: ", path);
    LOGGER((char*)"CLI_truncate: newsize=", (int)newsize);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;
    int truncate_ret = -1;

    // check open state
    int fmode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_truncate: open state=",fmode);
    retcode = fetch_if_needed(userdata, cache_path, path, true, fmode);
    RETURN_IF_NEG(retcode);

    fxn_ret = truncate_(cache_path, newsize);
    RETURN_IF_NEG(fxn_ret);

    // freshness check
    bool local = freshness_check(file_modes, cache_path, path);
    if (local) {
        LOGGER((char*)"------CLI_truncate end: success: ",fxn_ret);
        free(cache_path);
        return fxn_ret;
    }

    // synchronously write client's copy of the file to server
    retcode = upload(userdata, path);
    set_tc_to_now(file_modes, cache_path);
    RETURN_IF_NEG(retcode);

    free(cache_path);
    LOGGER((char*)"------CLI_truncate end: success: ", fxn_ret);
    return fxn_ret;
}

int watdfs_cli_fsync(void *userdata, const char *path, struct fuse_file_info *fi) {// Force a flush of file data.
    LOGGER((char*)"------CLI_fsync called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    int client_fd = file_modes_getfdc(file_modes, cache_path);
    severe_log_ifneg(client_fd, (char*)"CLI_write: can't get client_fd from map");
    fxn_ret = fsync_(cache_path, client_fd);
    RETURN_IF_NEG(fxn_ret);

    // immediately write back to server
    retcode = upload(userdata, path);
    RETURN_IF_NEG(retcode);

    // update tc
    set_tc_to_now(file_modes, cache_path);

    free(cache_path);
    LOGGER((char*)"------CLI_fsync end: success: ",fxn_ret);
    return fxn_ret;
}

// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path, const struct timespec ts[2]) {// Change file access and modification times.
    LOGGER((char*)"------CLI_utimens called: ", path);
    file_modes_t *file_modes = (file_modes_t *)userdata;
    char *cache_path = get_cache_path(path);
    int retcode, fxn_ret;

    // check open state
    int fmode = file_modes_getmode(file_modes, cache_path);
    LOGGER((char*)"CLI_utimens: open state=",fmode);
    retcode = fetch_if_needed(userdata, cache_path, path, true, fmode);
    RETURN_IF_NEG(retcode);

    // perform locally
    fxn_ret = utimensat(0, cache_path, ts, 0);
    if (fxn_ret < 0) return -errno;

    // freshness check
    bool local = freshness_check(file_modes, cache_path, path);
    if (local) {
        LOGGER((char*)"------CLI_utimens end: success: ",fxn_ret);
        free(cache_path);
        return fxn_ret;
    }

    // synchronously write client's copy of the file to server
    retcode = upload(userdata, path);
    set_tc_to_now(file_modes, cache_path);
    RETURN_IF_NEG(retcode);

    free(cache_path);
    LOGGER((char*)"------CLI_utimens end: success: ",fxn_ret);
    return fxn_ret;
}
