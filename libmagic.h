#include <stdio.h>
#include <magic.h> 
#include <string.h>

int istext(char* file_path){

    const char *magic_full;

    magic_t magic_cookie;

    magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING);

    if (magic_cookie == NULL) {
        return 1;
    }

    if (magic_load(magic_cookie, "/usr/share/misc/magic.mgc") != 0) {
        magic_close(magic_cookie);

        return 1;
    }

    magic_full = magic_file(magic_cookie, file_path);

    if (strncmp(magic_full, "cannot", 6) == 0)
        return 0; // error occured, not text
    int text = (strncmp(magic_full, "text", 4) == 0) ? 1:0; // 1 text, 0 non-text
    magic_close(magic_cookie);

    return text;
}

int isdir(char* file_path){

    const char *magic_full;

    magic_t magic_cookie;

    magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING);

    if (magic_cookie == NULL) {
        return 1;
    }

    if (magic_load(magic_cookie, "/usr/share/misc/magic.mgc") != 0) {
        magic_close(magic_cookie);

        return 1;
    }

    magic_full = magic_file(magic_cookie, file_path);

    if (strncmp(magic_full, "cannot", 6) == 0){
        return 0; // error occured, not dir
    }

    int dir = (strncmp(magic_full, "inode/directory", 15) == 0) ? 1:0; // 1 directory
    magic_close(magic_cookie);

    return dir;
}