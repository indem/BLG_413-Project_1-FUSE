#define FUSE_USE_VERSION 26
#define HELP 0

#include <stdio.h>
#include <fuse/fuse.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ansilove.h>
#include "libmagic.h"

char *src_path;

static char* translate_path(const char* path)
{
    char *rPath= malloc(sizeof(char)*(strlen(path)+strlen(src_path)+1));

    strcpy(rPath,src_path);
    if (rPath[strlen(rPath)-1]=='/') {
        rPath[strlen(rPath)-1]='\0';
    }
    strcat(rPath,path);

    return rPath;
}

void replace_png(char* upath){
    // if upath ends with .png, replace it with txt
    char* dotaddr = strrchr(upath, '.');
    int dotix = dotaddr - upath;

    if (dotaddr != NULL && !strcmp(dotaddr, ".png")){
        char* slashaddr = strrchr(upath, '/');
        int slashix = slashaddr - upath;
    
        char new_path[1000] = "";
        strncpy(new_path, upath, slashix + 1); // with slash

        DIR *dir;
        struct dirent *entry;
        if ((dir = opendir(new_path)) != NULL){
            char filename[1000] = "";
            strncpy(filename, upath + slashix + 1, dotix - slashix);
            
            while ((entry = readdir(dir)) != NULL){
                if (strncmp(entry->d_name, filename, strlen(filename)) == 0){
                    strcat(new_path, entry->d_name);
                    strcpy(upath, new_path);
                    break;
                }
            }
        }
    }
}

char* change_extension(char* orig_name){
    // if text file has extension: 
    int ext_start = 0;
    char * dotaddr = strrchr(orig_name, '.');
    if(dotaddr){
        ext_start = dotaddr - orig_name;
        // irem.txt ext_start = 4
        char* new_name;
        new_name = (char *)malloc(sizeof(char) * (ext_start + 5));
        strncpy(new_name, orig_name, ext_start);
        new_name[ext_start] = '\0';
        strncat(new_name, ".png", 4);

        return new_name;
    }
    char *new_name;
    new_name = (char *)malloc(sizeof(char)*(strlen(orig_name)+ 5 ));
    strncpy(new_name, orig_name, strlen(orig_name));
    strncat(new_name,".png", 4);

    return new_name;
}

struct ansilove_ctx* generate_png(char* upath){
	struct ansilove_ctx *ctx = malloc(sizeof(struct ansilove_ctx));
	struct ansilove_options options;

	ansilove_init(ctx, &options);
	ansilove_loadfile(ctx, upath);
	ansilove_ansi(ctx, &options);

    return ctx;
}

/******************************
*     Callbacks for FUSE      *   
******************************/

static int fs_getattr(const char *path, struct stat *st_data) {
    int res;
    char *upath=translate_path(path);

    // if upath ends with .png, replace it with txt
    replace_png(upath);
    res = lstat(upath, st_data);

    if (istext(upath)){
        struct ansilove_ctx *ctx = generate_png(upath);
        st_data->st_size = ctx->png.length * sizeof(uint8_t);
        ansilove_clean(ctx);
    }
 
    free(upath);
    if(res == -1) {
        return -errno;
    }

    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi) {
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
        strcpy(temp, upath);
        if (temp[strlen(temp) - 1] != '/')
            strcat(temp, "/");
        strcat(temp, de->d_name);

        if (istext(temp) == 1 || isdir(temp) == 1){
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            
            if (istext(temp) == 1){
                if (filler(buf, change_extension(de->d_name), &st, 0))
                    break;

            }
            else if (isdir(temp) == 1){
                if (filler(buf, de->d_name, &st, 0))
                     break;
            }
        }
    }

    closedir(dp);
    free(upath);

    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *finfo) {
    int res;
    int flags = finfo->flags;

    if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_CREAT) || (flags & O_EXCL) || (flags & O_TRUNC) || (flags & O_APPEND)) {
        return -EROFS;
    }

    char *upath=translate_path(path);
    res = open("/home/irem/Desktop/temp.png", flags);

    free(upath);
    if(res == -1) {
        return -errno;
    }

    return 0;
}


static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *finfo) {
    int res;
    (void)finfo;

    char *upath=translate_path(path);
    replace_png(upath);

    struct ansilove_ctx *ctx = generate_png(upath);

    memcpy(buf, ctx->png.buffer, ctx->png.length);
    res = ctx->png.length;

    if(res == -1) {
        res = -errno;
    }

    free(upath);
	ansilove_clean(ctx);
    return res;
}

struct fuse_operations fs_oper = {
    .getattr     = fs_getattr,
    .readdir     = fs_readdir,
    .open        = fs_open,
    .read        = fs_read,
};


static int fs_parse_opt(void *data, const char *arg, int key,
                          struct fuse_args *outargs)
{
    (void) data;

    switch (key)
    {
    case FUSE_OPT_KEY_NONOPT:
        if (src_path == 0){
            src_path = strdup(arg);
            return 0;
        }
        else
            return 1;
        
    case FUSE_OPT_KEY_OPT:
        return 1;
    case HELP:
        printf("To run:               ./filesystem file mountpoint    \n"
               "To run in debug mode: ./filesystem -d file mountpoint \n");
        exit(0);
    default:
        printf("Invalid command.\nTry ./filename -h\n");
        exit(1);
    }
    return 1;
}

static struct fuse_opt fs_opts[] = {
    FUSE_OPT_KEY("-h",          HELP),
    FUSE_OPT_KEY("--help",      HELP),
    FUSE_OPT_END
};

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res;

    res = fuse_opt_parse(&args, &src_path, fs_opts, fs_parse_opt);
    if (res != 0) {
        printf("Invalid arguments\n");
        printf("Try ./filename -h\n");
        exit(1);
    }

    if (src_path == 0) {
        printf("Invalid command.\n");
        printf("Try ./filename -h\n");
        exit(1);
    }

    fuse_main(args.argc, args.argv, &fs_oper, NULL);

    return 0;
}
