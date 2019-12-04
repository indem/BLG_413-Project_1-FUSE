#include <stdio.h>

#include <magic.h> 

#include <string.h>

int istext(char* file_path){

    const char *magic_full;

    magic_t magic_cookie;


    /* MAGIC_MIME tells magic to return a mime of the file, 

       but you can specify different things	*/

    magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING);

    if (magic_cookie == NULL) {

        printf("unable to initialize magic library\n");

        return 1;

    }


    printf("Loading default magic database\n");
    

    if (magic_load(magic_cookie, "/usr/share/misc/magic.mgc") != 0) {

        printf("cannot load magic database - %s\n", magic_error(magic_cookie));

        magic_close(magic_cookie);

        return 1;

    }

    magic_full = magic_file(magic_cookie, file_path);
    printf("%s\n", magic_full);
    int text = (strncmp(magic_full, "text", 4) == 0) ? 1:0; // 1 text, 0 non-text
    int dir = (strncmp(magic_full, "inode/directory", 15) == 0) ? 1:0; // 1 directory
    magic_close(magic_cookie);

    return text || dir;
}