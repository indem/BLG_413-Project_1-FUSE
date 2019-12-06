/*
 * fs - The read-only filesystem for FUSE.
 * Copyright 2005,2006,2008 Matthew Keller. kellermg@potsdam.edu and others.
 * v2008.09.24
 *
 * Mount any filesytem, or folder tree read-only, anywhere else.
 * No warranties. No guarantees. No lawyers.
 *
 * I read (and borrowed) a lot of other FUSE code to write this.
 * Similarities possibly exist- Wholesale reuse as well of other GPL code.
 * Special mention to Rémi Flament and his loggedfs.
 *
 * Consider this code GPLv2.
 *
 * Compile: gcc -o fs -Wall -ansi -W -std=c99 -g -ggdb -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -lfuse fs.c
 * Mount: fs readwrite_filesystem mount_point
 *
 */


#define FUSE_USE_VERSION 26

static const char* fsVersion = "2008.09.24";

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <dirent.h>
#include <unistd.h>
#include <fuse/fuse.h>
#include "libmagic.h"
#include <ansilove.h>

// Global to store our read-write path
char *rw_path;

// Translate an fs path into it's underlying filesystem path
static char* translate_path(const char* path)
{

    char *rPath= malloc(sizeof(char)*(strlen(path)+strlen(rw_path)+1));

    strcpy(rPath,rw_path);
    if (rPath[strlen(rPath)-1]=='/') {
        rPath[strlen(rPath)-1]='\0';
    }
    strcat(rPath,path);

    return rPath;
}


/******************************
*
* Callbacks for FUSE
*
*
*
******************************/

static int fs_getattr(const char *path, struct stat *st_data)
{
    int res;
    char *upath=translate_path(path);
    res = lstat(upath, st_data);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    return 0;
}

static int fs_readlink(const char *path, char *buf, size_t size)
{
    int res;
    char *upath=translate_path(path);

    res = readlink(upath, buf, size - 1);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    buf[res] = '\0';
    return 0;
}

char* change_extension(char* orig_name){
    //if text file has extension: 
    int ext_start = 0;
    char * dotaddr = strrchr(orig_name, '.');
    if(dotaddr){
        ext_start = dotaddr - orig_name;
        // irem.txt ext_start = 4
        char* new_name;
        new_name = (char *)malloc(sizeof(char) * (ext_start + 5));
        memset(new_name, '\0', sizeof(new_name));
        strncpy(new_name, orig_name, ext_start);
        new_name[ext_start] = '\0';
        strncat(new_name, ".png", 4);

        return new_name;
    }
    char *new_name;
    new_name = (char *)malloc(sizeof(char)*(strlen(orig_name)+ 5 ));
    memset(new_name, '\0', sizeof(new_name));
    strncpy(new_name, orig_name, strlen(orig_name));
    strncat(new_name,".png", 4);

    return new_name;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int res;

    (void) offset;
    (void) fi;

    char *upath=translate_path(path);

    dp = opendir(upath);

    if(dp == NULL) {
        res = -errno;
        return res;
    }


    while((de = readdir(dp)) != NULL) {
        char temp[1000] = "";
        strcpy(temp, de->d_name);
        char* new_name = change_extension(temp);
        strcpy(temp, upath);
        if (temp[strlen(temp) - 1] != '/')
            strcat(temp, "/");
        strcat(temp, de->d_name);
 
        printf("TEMP: %s\n", temp);
        int i = istext(temp);

        if (istext(temp) == 1 || isdir(temp) == 1){
            printf("d->d_name: %s\n", de->d_name);
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            if (filler(buf, de->d_name, &st, 0))
                 break;
        }

    }
    closedir(dp);
    free(upath);
    return 0;
    
}
static int fs_open(const char *path, struct fuse_file_info *finfo)
{
    int res;

    /* We allow opens, unless they're tring to write, sneaky
     * people.
     */
    int flags = finfo->flags;

    if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_CREAT) || (flags & O_EXCL) || (flags & O_TRUNC) || (flags & O_APPEND)) {
        return -EROFS;
    }

    char *upath=translate_path(path);

    res = open(upath, flags);

    free(upath);
    if(res == -1) {
        return -errno;
    }
    close(res);
    return 0;
}

static int fs_read(const char *path, uint8_t *buf, size_t size, off_t offset, struct fuse_file_info *finfo)
{
    int fd = 0;
    fd += 1;
    int res = 0;
    (void)finfo;

    struct ansilove_ctx ctx;
    struct ansilove_options options;

    char *upath=translate_path(path);

    ansilove_init(&ctx, &options);
    ansilove_loadfile(&ctx, upath);
    ansilove_ansi(&ctx, &options);
    ansilove_savefile(&ctx, "/tmp/example.png");

    for (int i = 0; i < ctx.png.length; i++){
        buf[i] = ctx.png.buffer[i];
    }
    size = ctx.png.length;
    ansilove_clean(&ctx);
    return res;
}


static int fs_statfs(const char *path, struct statvfs *st_buf)
{
    int res;
    char *upath=translate_path(path);
    res = statvfs(upath, st_buf);
    free(upath);
    if (res == -1) {
        return -errno;
    }
    return 0;
}


static int fs_access(const char *path, int mode)
{
    int res;
    char *upath=translate_path(path);

    /* Don't pretend that we allow writing
     * Chris AtLee <chris@atlee.ca>
     */
    if (mode & W_OK)
        return -EROFS;

    res = access(upath, mode);
    free(upath);
    if (res == -1) {
        return -errno;
    }
    return res;
}


/*
 * Get the value of an extended attribute.
 */
static int fs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int res;

    char *upath=translate_path(path);
    res = lgetxattr(upath, name, value, size);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    return res;
}

/*
 * List the supported extended attributes.
 */
static int fs_listxattr(const char *path, char *list, size_t size)
{
    int res;
    char *upath=translate_path(path);
    res = llistxattr(upath, list, size);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    return res;

}

struct fuse_operations fs_oper = {
    .getattr     = fs_getattr,
    .readlink    = fs_readlink,
    .readdir     = fs_readdir,
    .open        = fs_open,
    .read        = fs_read,
    .statfs      = fs_statfs,
    .access      = fs_access,
    /* Extended attributes support for userland interaction */
    .getxattr    = fs_getxattr,
    .listxattr   = fs_listxattr,
};
enum {
    KEY_HELP,
    KEY_VERSION,
};

static void usage(const char* progname)
{
    fprintf(stdout,
            "usage: %s readwritepath mountpoint [options]\n"
            "\n"
            "   Mounts readwritepath as a read-only mount at mountpoint\n"
            "\n"
            "general options:\n"
            "   -o opt,[opt...]     mount options\n"
            "   -h  --help          print help\n"
            "   -V  --version       print version\n"
            "\n", progname);
}

static int fs_parse_opt(void *data, const char *arg, int key,
                          struct fuse_args *outargs)
{
    (void) data;

    switch (key)
    {
    case FUSE_OPT_KEY_NONOPT:
        if (rw_path == 0)
        {
            rw_path = strdup(arg);
            return 0;
        }
        else
        {
            return 1;
        }
    case FUSE_OPT_KEY_OPT:
        return 1;
    case KEY_HELP:
        usage(outargs->argv[0]);
        exit(0);
    case KEY_VERSION:
        fprintf(stdout, "fs version %s\n", fsVersion);
        exit(0);
    default:
        fprintf(stderr, "see `%s -h' for usage\n", outargs->argv[0]);
        exit(1);
    }
    return 1;
}

static struct fuse_opt fs_opts[] = {
    FUSE_OPT_KEY("-h",          KEY_HELP),
    FUSE_OPT_KEY("--help",      KEY_HELP),
    FUSE_OPT_KEY("-V",          KEY_VERSION),
    FUSE_OPT_KEY("--version",   KEY_VERSION),
    FUSE_OPT_END
};

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res;

    res = fuse_opt_parse(&args, &rw_path, fs_opts, fs_parse_opt);
    if (res != 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "see `%s -h' for usage\n", argv[0]);
        exit(1);
    }
    if (rw_path == 0)
    {
        fprintf(stderr, "Missing readwritepath\n");
        fprintf(stderr, "see `%s -h' for usage\n", argv[0]);
        exit(1);
    }

    fuse_main(args.argc, args.argv, &fs_oper, NULL);

    return 0;
}
