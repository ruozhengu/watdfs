//
// The interface that must be implemented to support WatDFS.
// Starter code for CS 454/654.
// DO NOT change this file.
//

#ifndef WATDFS_CLIENT_H
#define WATDFS_CLIENT_H

// The FUSE API has been changed a number of times.  Our code needs to define
// the version of the API that we assume.  As of this writing, a stable API
// version is 26.
#define FUSE_USE_VERSION 26

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

#ifdef __cplusplus
// This is for backward compatibility, but all your code *must* be in C++.
extern "C" {
#endif

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *retcode);
void watdfs_cli_destroy(void *userdata);

// GET FILE ATTRIBUTES
int watdfs_cli_fgetattr(void *userdata, const char *path, struct stat *statbuf,
                        struct fuse_file_info *fi);
int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf);

// CREATE, OPEN AND CLOSE
int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev);
int watdfs_cli_open(void *userdata, const char *path,
                    struct fuse_file_info *fi);
int watdfs_cli_release(void *userdata, const char *path,
                       struct fuse_file_info *fi);

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi);
int watdfs_cli_write(void *userdata, const char *path, const char *buf,
                     size_t size, off_t offset, struct fuse_file_info *fi);
int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize);
int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi);

// CHANGE METADATA
int watdfs_cli_utimens(void *userdata, const char *path,
                       const struct timespec ts[2]);

#ifdef __cplusplus
}
#endif

#endif
