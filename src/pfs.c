/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall pfs.c `pkg-config fuse --cflags --libs` -o pfs
*/

#define FUSE_USE_VERSION 26

#include "pfs.h"

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

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void pfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, PRI_DATA->rootdir);
    strncat(fpath, path, PATH_MAX);
}


static int pfs_getattr(const char *path, struct stat *stbuf)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	pfs_fullpath(fpath,path);
	
	retstat = lstat(fpath,stbuf);
	if(retstat != 0){
		retstat = -errno;
	}
	return retstat;
}

int pfs_readlink(const char* path, char* link, size_t size){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	
	retstat = readlink(fpath, link, size - 1);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

int pfs_mknod(const char* path, mode_t mode, dev_t dev){
	int retstat;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	if(S_ISREG(mode)){
		retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if(retstat < 0){
			retstat = -errno;
		}
		else{
			retstat = close(retstat);
			if(retstat < 0){
				retstat = -errno;
			}
		}
	}
	else if(S_ISFIFO(mode)){
		retstat = mkfifo(fpath, mode);
		if(retstat < 0){
			retstat = -errno;
		}
	}
	else{
		retstat = mknod(fpath, mode, dev);
		if(retstat < 0){
			retstat = -errno;
		}
	}
	
	return retstat;
}

int pfs_mkdir(const char* path, mode_t mode){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = mkdir(fpath,mode);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

int pfs_unlink(const char* path){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = unlink(fpath);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

int pfs_rmdir(const char* path){
	  int retstat = 0;
	  char fpath[PATH_MAX];
	  
	  pfs_fullpath(fpath, path);
	  
	  retstat = rmdir(fpath);
	  if(retstat < 0){
	  	retstat = -errno;
	  }
	  
	  return retstat;
}

int pfs_symlink(const char *path, const char *link)
{
    int retstat = 0;
    char flink[PATH_MAX];
    
	pfs_fullpath(flink, link);    
	
    retstat = symlink(path, flink);
    if (retstat < 0){
		retstat = -errno;
	}
    return retstat;
}

int pfs_rename(const char* path, const char* newpath){
	int retstat = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	pfs_fullpath(fnewpath, newpath);
	
	retstat = rename(fpath, fnewpath);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

int pfs_link(const char* path, const char* newpath){
	int retstat = 0;
	char fpath[PATH_MAX], fnewpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	pfs_fullpath(fnewpath,newpath);
	
	retstat = link(fpath, fnewpath);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

/** Change the permission bits of a file */
int pfs_chmod(const char *path, mode_t mode)
{
    int retstat;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath,path);
    retstat = chmod(path, mode);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}

int pfs_chown(const char* path, uid_t uid, gid_t gid){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = chown(fpath, uid, gid);
	if(retstat < 0){
		retstat = -errno;
	}
	
	return retstat;
}

int pfs_truncate(const char* path, off_t newsize){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = truncate(fpath, newsize);
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}

int pfs_utime(const char* path, struct utimbuf *ubuf){
	int retstat;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = utime(fpath,ubuf);
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}

static int pfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	fd = open(fpath, fi->flags);
	if(fd < 0){
		retstat = -errno;
	}
	
	fi->fh = fd;
	
	return retstat;
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int retstat = pread(fi->fh, buf, size, offset);
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}

int pfs_write(const char* path, const char* buf, size_t size, off_t offset, 
				struct fuse_file_info* fi)
{
	 int retstat = 0;
	 
	 retstat = pwrite(fi->fh, buf, size, offset);
	 if(retstat < 0) retstat = -errno;
	 
	 return retstat;				
}

int pfs_statfs(const char* path, struct statvfs* statv){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = statvfs(fpath, statv);
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}

int pfs_flush(const char* path, struct fuse_file_info* fi){
	int retstat = 0;
	
	return retstat;
}

int pfs_release(const char* path, struct fuse_file_info* fi){
	int retstat = 0;
	retstat = close(fi->fh);
	return retstat;
}

int pfs_fsync(const char* path, int datasync, struct fuse_file_info* fi){
	int retstat = 0;
#ifdef HAVE_FDATASYNC
	if(datasync){
		retstat = fdatasync(fi->fh);
	}
	else
#endif
	retstat = fsync(fi->fh);
	
	if(retstat < 0) retstat = -errno;
	
	return retstat;	
}

#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
int pfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}

/** Get extended attributes */
int pfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}

/** List extended attributes */
int pfs_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    
    pfs_fullpath(fpath, path);
    
    retstat = llistxattr(fpath, list, size);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}

/** Remove extended attributes */
int pfs_removexattr(const char *path, const char *name)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lremovexattr(fpath, name);
    if (retstat < 0)
	retstat = -errno;
    
    return retstat;
}
#endif

int pfs_opendir(const char* path, struct fuse_file_info* fi){
	DIR *dp;
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	dp = opendir(fpath);
	if(dp == NULL){
		retstat = -errno;
	}
	
	fi->fh = (intptr_t) dp;
	return retstat;
}

int pfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, 
					struct fuse_file_info* fi)
{
	int retstat = 0;
	DIR* dp;
	struct dirent* de;
	
	dp = (DIR*) (uintptr_t) fi->fh;
	
	de = readdir(dp);
	if(de == 0){
		retstat = -errno;
		return retstat;
	}
	
	do{
		if(filler(buf,de->d_name, NULL, 0) != 0) {
			return -ENOMEM;
		}
	} while((de = readdir(dp)) != NULL);
	
	return retstat;
}

int pfs_releasedir(const char* path, struct fuse_file_info* fi){
	int retstat = 0;
	
	closedir((DIR*)(uintptr_t)fi->fh);
	return retstat;
}

int pfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi){
	return 0;
}

void* pfs_init(struct fuse_conn_info *conn){
	return PRI_DATA;
}

void pfs_destroy(void* userdata){}

int pfs_access(const char* path, int mask){
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = access(fpath,mask);
	
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}

int pfs_create(const char* path, mode_t mode, struct fuse_file_info* fi){
	int retstat = 0;
	char fpath[PATH_MAX];
	int fd;
	
	pfs_fullpath(fpath,path);
	
	fd = creat(fpath, mode);
	if(fd < 0) retstat = -errno;
	
	fi->fh = fd;
	
	return retstat;
}

int pfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi){
	int retstat = 0;
	retstat = ftruncate(fi->fh, offset);
	if(retstat < 0) retstat = -errno;
	return retstat;
}

int pfs_fgetattr(const char* path, struct stat* statbuf, struct fuse_file_info* fi){
	int retstat = 0;
	
	if(!strcmp(path,"/")){
		return pfs_getattr(path,statbuf);
	}
	
	retstat = fstat(fi->fh, statbuf);
	if(retstat < 0) retstat = -errno;
	
	return retstat;
}



struct fuse_operations pfs_oper = {
  .getattr = pfs_getattr,
  .readlink = pfs_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = pfs_mknod,
  .mkdir = pfs_mkdir,
  .unlink = pfs_unlink,
  .rmdir = pfs_rmdir,
  .symlink = pfs_symlink,
  .rename = pfs_rename,
  .link = pfs_link,
  .chmod = pfs_chmod,
  .chown = pfs_chown,
  .truncate = pfs_truncate,
  .utime = pfs_utime,
  .open = pfs_open,
  .read = pfs_read,
  .write = pfs_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = pfs_statfs,
  .flush = pfs_flush,
  .release = pfs_release,
  .fsync = pfs_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = pfs_setxattr,
  .getxattr = pfs_getxattr,
  .listxattr = pfs_listxattr,
  .removexattr = pfs_removexattr,
#endif
  
  .opendir = pfs_opendir,
  .readdir = pfs_readdir,
  .releasedir = pfs_releasedir,
  .fsyncdir = pfs_fsyncdir,
  .init = pfs_init,
  .destroy = pfs_destroy,
  .access = pfs_access,
  .create = pfs_create,
  .ftruncate = pfs_ftruncate,
  .fgetattr = pfs_fgetattr
};

int main(int argc, char *argv[])
{
	if(argc < 3){
		fprintf(stderr, "usage: [FUSE and mount options] rootDir mountPoint\n");
		return 0;
	}
	int fuse_stat;
	struct state* data = calloc(1,sizeof(struct state));
	
	data->rootdir = realpath(argv[argc-2],NULL);
	argv[argc-2] = argv[argc-1];
	argv[argc-1] = NULL;
	argc--;
	
	printf("PATH_MAX is:%d\n",PATH_MAX);
	fuse_stat = fuse_main(argc, argv, &pfs_oper, data);
	return fuse_stat;
}




















