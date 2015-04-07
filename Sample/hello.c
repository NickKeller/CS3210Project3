/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello.txt";


static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	printf("Entered hello_getattr\n");
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, hello_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	printf("Entered hello_readdir\n");
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, hello_path + 1, NULL, 0);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	printf("Entered hello_open\n");
	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	printf("Entered hello_read\n");
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

static int hello_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	int retstat = 0;
	int fd = creat(path, mode);
	if(fd < 0){
		retstat = -errno;
	}
	
	fi->fh = fd;
	
	return retstat;

}

static int hello_unlink(const char *path){
	int retstat = 0;
	retstat = unlink(path);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

static void hello_destroy(void *userdata){
	
}

static int hello_rename(const char *path, const char *newpath){
	int retstat = rename(path,newpath);
	if(retstat < 0)
		retstat = -errno;
	return retstat;
}

/** Change the permission bits of a file */
int hello_chmod(const char *path, mode_t mode)
{
    int retstat = chmod(path, mode);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}



static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
	.create		= hello_create,
	.unlink		= hello_unlink,
	.destroy	= hello_destroy,
	.rename		= hello_rename,
	.chmod		= hello_chmod
};

int main(int argc, char *argv[])
{
	printf("PATH_MAX is:%d\n",PATH_MAX);
	return fuse_main(argc, argv, &hello_oper, NULL);
}
