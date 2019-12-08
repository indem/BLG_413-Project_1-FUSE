# PROJECT 2: FUSE
**Ezgi Paket**, 070140122 | **İrem Nur Demirtaş**, 150150302 | **Ayser Ecem Konu**, 150160711 

## Required Libraries
* libfuse
* libansilove 

## Compilation and Running
cmake is required for compilation.
1.  Create a new directory named build:
    
    ```mkdir build```

2.  Enter the newly created directory.

    ```cd build```

3.  Run the following command. It will create a Makefile in the current directory. 

    ```cmake "Unix Makefile" ..```

4.  Run the following command. The Makefile will produce an executable named "filesystem" in the current directory.

    ```make```
    
5a.  Run the following command to initialize the file system in debug mode. In debug mode, some relevant messages will be printed.

    ```./filesystem -d <source file> <mount point>```

5b.  Or just run the following command to initialize the file system in normal mode. Messages won't be printed.

    ```./filesystem  <source file> <mount point>```

## Usage
While the program is running, you will be able to see PNG versions of text files in the source directory when you visit the mount point, as if you are scrolling through any other directory on your computer. Any program that can open images should be able to show the images.