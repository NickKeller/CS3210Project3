#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

// maintain pfs state in here
#include <limits.h>
#include <stdio.h>
#include <fuse.h>
struct state {
    FILE *logfile;
    char *rootdir;
    int numMounts;
    int master;
    char* backup;
};

//hash function stuff
unsigned long hashFunction(char *str);
void addNode(char *mount);
void removeNode(char *mount);
struct node *search(char *key);
void printList();
int getSize();
struct node
{
    unsigned long hash;
    char *mount;
    struct node *next;
};

//database stuff
#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/my_config.h>
#include <mysql/my_global.h>

int deleteImage(char path[]);
int updatePath( char newPath[], char oldPath[]);
int insertImage(char path[]);


int mapNameToDrives(const char* path);
struct fuse *setup_common(int argc, char *argv[],const struct fuse_operations *op,size_t op_size,char **mountpoint,int *multithreaded,int *fd, void *user_data,int compat);
#define PRI_DATA ((struct state *) fuse_get_context()->private_data)

#endif
