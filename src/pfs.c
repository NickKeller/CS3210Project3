/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall pfs.c `pkg-config fuse --cflags --libs` -o pfs
*/

#include "pfs.h"
#include "log.h"

#include "config.h"
#include <fuse_opt.h>
#include <fuse_lowlevel.h>
#include <fuse_common_compat.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <fuse_common.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

char* mapNameToDrives(const char* path){
	log_msg("Entered mapNameToDrives, path is: %s\n",path);
	return "ABCDE";
}

static int pfs_error(char* str){
	int ret = -errno;
	log_msg("	ERROR %s: %s\n",str,strerror(errno));
	return ret;
}

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void pfs_fullpath(char fpath[PATH_MAX], const char *path)
{	
	log_msg("Entered pfs_fullpath\n");
    strcpy(fpath, PRI_DATA->rootdir);
    //strcpy(fpath,"~/MyPFS");
    strncat(fpath, path, PATH_MAX);
    
    log_msg("    pfs_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
    PRI_DATA->rootdir, path, fpath);
}


static int pfs_getattr(const char *path, struct stat *stbuf)
{
	log_msg("Entered pfs_getattr\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	pfs_fullpath(fpath,path);
	retstat = lstat(fpath,stbuf);
	if(retstat != 0){
		retstat = pfs_error("pfs_getattr lstat");
	}
	return retstat;
}

int pfs_readlink(const char* path, char* link, size_t size){
	log_msg("Entered pfs_readlink\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	
	retstat = readlink(fpath, link, size - 1);
	if(retstat < 0){
		retstat = pfs_error("pfs_readlink readlink");
	}
	
	return retstat;
}

int pfs_mknod(const char* path, mode_t mode, dev_t dev){
	log_msg("Entered pfs_mknod\n");
	int retstat;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	if(S_ISREG(mode)){
		retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if(retstat < 0){
			retstat = pfs_error("pfs_mknod open");
		}
		else{
			retstat = close(retstat);
			if(retstat < 0){
				retstat = pfs_error("pfs_mknod close");
			}
		}
	}
	else if(S_ISFIFO(mode)){
		retstat = mkfifo(fpath, mode);
		if(retstat < 0){
			retstat = pfs_error("pfs_mknod mkfifo");
		}
	}
	else{
		retstat = mknod(fpath, mode, dev);
		if(retstat < 0){
			retstat = pfs_error("pfs_mknod mknod");
		}
	}
	
	return retstat;
}

int pfs_mkdir(const char* path, mode_t mode){
	log_msg("Entered pfs_mkdir\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	retstat = mkdir(fpath,mode);	
	
	//backup
	log_msg("Making dir: %s\n",path);
/*	char* drives = mapNameToDrives(path);
	for(int i = 0; i < strlen(drives); i++){
		char* newpath = calloc(PATH_MAX,sizeof(char));
		strcpy(newpath,PRI_DATA->backupdir);
		log_msg("path is %s\n",newpath);
		strncat(newpath,"/",1);
		log_msg("path is %s\n",newpath);
		strncat(newpath,&drives[i],1);
		log_msg("path is %s\n",newpath);
		strncat(newpath,path,strlen(path));
		log_msg("path is %s\n",newpath);
		mkdir(newpath,mode);
	}*/
	
	if(retstat < 0){
		retstat = pfs_error("pfs_mkdir mkdir");
	}
	
	return retstat;
}

static int pfs_unlink(const char* path){
	log_msg("Entered pfs_unlink\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = unlink(fpath);
	if(retstat < 0){
		retstat = pfs_error("pfs_unlink unlink");
	}
	
	return retstat;
}

static int pfs_rmdir(const char* path){
	log_msg("Entered pfs_rmdir\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = rmdir(fpath);
	if(retstat < 0){
		retstat = pfs_error("pfs_rmdir rmdir");
	}
	
	return retstat;
}

static int pfs_symlink(const char *path, const char *link)
{
	log_msg("Entered pfs_symlink\n");
    int retstat = 0;
    char flink[PATH_MAX];
    
	pfs_fullpath(flink, link);    
	
    retstat = symlink(path, flink);
    if (retstat < 0){
		retstat = pfs_error("pfs_symlink symlink");
	}
    return retstat;
}

static int pfs_rename(const char* path, const char* newpath){
	log_msg("Entered pfs_rename\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	pfs_fullpath(fnewpath, newpath);
	
	retstat = rename(fpath, fnewpath);
	if(retstat < 0){
		retstat = pfs_error("pfs_rename rename");
	}
	
	return retstat;
}

static int pfs_link(const char* path, const char* newpath){
	log_msg("Entered pfs_link\n");
	int retstat = 0;
	char fpath[PATH_MAX], fnewpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	pfs_fullpath(fnewpath,newpath);
	
	retstat = link(fpath, fnewpath);
	if(retstat < 0){
		retstat = pfs_error("pfs_link link");
	}
	
	return retstat;
}

/** Change the permission bits of a file */
static int pfs_chmod(const char *path, mode_t mode)
{
	log_msg("Entered pfs_chmod\n");
    int retstat;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath,path);
    retstat = chmod(path, mode);
    if (retstat < 0)
	retstat = pfs_error("pfs_chmod chmod");
    
    return retstat;
}

static int pfs_chown(const char* path, uid_t uid, gid_t gid){
	log_msg("Entered pfs_chown\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = chown(fpath, uid, gid);
	if(retstat < 0){
		retstat = pfs_error("pfs_chown chown");
	}
	
	return retstat;
}

static int pfs_truncate(const char* path, off_t newsize){
	log_msg("Entered pfs_truncate\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = truncate(fpath, newsize);
	if(retstat < 0) retstat = pfs_error("pfs_truncate truncate");
	
	return retstat;
}

static int pfs_utime(const char* path, struct utimbuf *ubuf){
	log_msg("Entered pfs_utime\n");
	int retstat;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = utime(fpath,ubuf);
	if(retstat < 0) retstat = pfs_error("pfs_utime utime");
	
	return retstat;
}

static int pfs_open(const char *path, struct fuse_file_info *fi)
{
	log_msg("Entered pfs_open\n");
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	fd = open(fpath, fi->flags);
	if(fd < 0){
		retstat = pfs_error("pfs_open open");
	}
	
	fi->fh = fd;
	
	return retstat;
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	log_msg("Entered pfs_read\n");
	int retstat = pread(fi->fh, buf, size, offset);
	if(retstat < 0) retstat = pfs_error("pfs_read read");
	
	return retstat;
}

static int pfs_write(const char* path, const char* buf, size_t size, off_t offset, 
				struct fuse_file_info* fi)
{
	log_msg("Entered pfs_write\n");
	 int retstat = 0;
	 
	 retstat = pwrite(fi->fh, buf, size, offset);
	 if(retstat < 0) retstat = pfs_error("pfs_write pwrite");
	 
	 return retstat;				
}

static int pfs_statfs(const char* path, struct statvfs* statv){
	log_msg("Entered pfs_statfs\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath, path);
	
	retstat = statvfs(fpath, statv);
	if(retstat < 0) retstat = pfs_error("pfs_statfs statvfs");
	
	return retstat;
}

static int pfs_flush(const char* path, struct fuse_file_info* fi){
	log_msg("Entered pfs_flush\n");
	int retstat = 0;
	
	return retstat;
}

static int pfs_release(const char* path, struct fuse_file_info* fi){
	log_msg("Entered pfs_release\n");
	int retstat = 0;
	retstat = close(fi->fh);
	return retstat;
}

static int pfs_fsync(const char* path, int datasync, struct fuse_file_info* fi){
	log_msg("Entered pfs_fsync\n");
	int retstat = 0;
#ifdef HAVE_FDATASYNC
	if(datasync){
		retstat = fdatasync(fi->fh);
	}
	else
#endif
	retstat = fsync(fi->fh);
	
	if(retstat < 0) retstat = pfs_error("pfs_fsync fsync");
	
	return retstat;	
}

#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
static int pfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	log_msg("Entered pfs_setxattr\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0)
	retstat = pfs_error("pfs_setxattr lsetxattr");
    
    return retstat;
}

/** Get extended attributes */
static int pfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
	log_msg("Entered pfs_getxattr\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0)
	retstat = pfs_error("pfs_getxattr lgetxattr");
    
    return retstat;
}

/** List extended attributes */
static int pfs_listxattr(const char *path, char *list, size_t size)
{
	log_msg("Entered pfs_listxattr\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    
    pfs_fullpath(fpath, path);
    
    retstat = llistxattr(fpath, list, size);
    if (retstat < 0)
	retstat = pfs_error("pfs_listxattr llistxattr");
    
    return retstat;
}

/** Remove extended attributes */
static int pfs_removexattr(const char *path, const char *name)
{
	log_msg("Entered pfs_removexattr\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    
    pfs_fullpath(fpath, path);
    
    retstat = lremovexattr(fpath, name);
    if (retstat < 0)
	retstat = pfs_error("pfs_removexattr lremovexattr");
    
    return retstat;
}
#endif

static int pfs_opendir(const char* path, struct fuse_file_info* fi){
	log_msg("Entered pfs_opendir\n");
	DIR *dp;
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	dp = opendir(fpath);
	if(dp == NULL){
		retstat = pfs_error("pfs_opendir opendir");
	}
	
	fi->fh = (intptr_t) dp;
	return retstat;
}

static int pfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, 
					struct fuse_file_info* fi)
{
	log_msg("Entered pfs_readdir\n");
	int retstat = 0;
	DIR* dp;
	struct dirent* de;
	
	dp = (DIR*) (uintptr_t) fi->fh;
	
	de = readdir(dp);
	if(de == 0){
		retstat = pfs_error("pfs_readdir readdir");
		return retstat;
	}
	
	do{
		if(filler(buf,de->d_name, NULL, 0) != 0) {
			return -ENOMEM;
		}
	} while((de = readdir(dp)) != NULL);
	
	return retstat;
}

static int pfs_releasedir(const char* path, struct fuse_file_info* fi){
	log_msg("Entered pfs_releasedir\n");
	int retstat = 0;
	
	closedir((DIR*)(uintptr_t)fi->fh);
	return retstat;
}

static int pfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi){
	log_msg("Entered pfs_fsyncdir\n");
	return 0;
}

void* pfs_init(struct fuse_conn_info *conn){
	log_msg("Entered pfs_init\n");
	return PRI_DATA;
}

void pfs_destroy(void* userdata){
	log_msg("Entered pfs_destroy\n");
}

static int pfs_access(const char* path, int mask){
	log_msg("Entered pfs_access\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = access(fpath,mask);
	
	if(retstat < 0) retstat = pfs_error("pfs_access access");
	
	return retstat;
}

static int pfs_create(const char* path, mode_t mode, struct fuse_file_info* fi){
	log_msg("Entered pfs_create\n");
	int retstat = 0;
	char fpath[PATH_MAX];
	int fd;
	
	pfs_fullpath(fpath,path);
	
	fd = creat(fpath, mode);
	if(fd < 0) retstat = pfs_error("pfs_create creat");
	
	fi->fh = fd;
	
	return retstat;
}

static int pfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi){
	log_msg("Entered pfs_ftruncate\n");
	int retstat = 0;
	retstat = ftruncate(fi->fh, offset);
	if(retstat < 0) retstat = pfs_error("pfs_ftruncate ftruncate");
	return retstat;
}

static int pfs_fgetattr(const char* path, struct stat* statbuf, struct fuse_file_info* fi){
	log_msg("Entered pfs_fgetattr\n");
	int retstat = 0;
	
	if(!strcmp(path,"/")){
		return pfs_getattr(path,statbuf);
	}
	
	retstat = fstat(fi->fh, statbuf);
	if(retstat < 0) retstat = pfs_error("pfs_fgetattr fstat");
	
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


struct fuse *setup_common(int argc, char *argv[],
			       const struct fuse_operations *op,
			       size_t op_size,
			       char **mountpoint,
			       int *multithreaded,
			       int *fd,
			       void *user_data,
			       int compat)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	struct fuse *fuse;
	int foreground;
	int res;

	res = fuse_parse_cmdline(&args, mountpoint, multithreaded, &foreground);
	if (res == -1)
		return NULL;

	ch = fuse_mount(*mountpoint, &args);
	if (!ch) {
		fuse_opt_free_args(&args);
		goto err_free;
	}
	fprintf(stderr, "Calling fuse_new\n");
	fuse = fuse_new(ch, &args, op, op_size, user_data);
	fuse_opt_free_args(&args);
	if (fuse == NULL)
		goto err_unmount;
	fprintf(stderr, "calling fuse_daemonize\n");
	res = fuse_daemonize(foreground);
	if (res == -1)
		goto err_unmount;
	fprintf(stderr, "calling set_signal_handlers\n");
	res = fuse_set_signal_handlers(fuse_get_session(fuse));
	if (res == -1)
		goto err_unmount;

	if (fd)
		*fd = fuse_chan_fd(ch);
	fprintf(stderr, "returning fuse\n");
	return fuse;

err_unmount:
	fuse_unmount(*mountpoint, ch);
	if (fuse)
		fuse_destroy(fuse);
err_free:
	free(*mountpoint);
	return NULL;
}

int main(int argc, char *argv[])
{
	if(argc < 3){
		fprintf(stderr, "usage: pfs [FUSE and mount options] backupDir mountPoint\n");
		return 0;
	}
	struct state* data = calloc(1,sizeof(struct state));
//	struct state* data2 = calloc(1,sizeof(struct state));
	
	data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
	
	printf("MountDir is: %s\n",argv[argc-1]);
	
	
	printf("Rootdir is: %s\n",data->rootdir);//,data->backupdir);
	printf("Argc:%d\n",argc);
	for(int i = 0; i < argc; i++){
		printf("Argv[%d]:%s\n",i,argv[i]);
	}
	//exit(0);
	data->logfile = log_open("pfs.log");
	
	fprintf(stderr,"About to call fuse_main\n");
	
	
	struct fuse *fuse1;
//	struct fuse* fuse2;
	char *mountpoint1;
//	char* mountpoint2;
	int multithreaded1;
//	int multithreaded2;
	int res1;
//	int res2;
	
/*	char* argv2[] = {"./pfs",realpath("~/MyPFS2",NULL)};
	data2->rootdir = realpath("../backup/B/",NULL);
	data2->logfile = log_open("pfs2.log");*/
	
	
	fuse1 = setup_common(argc, argv, &pfs_oper, sizeof(pfs_oper), &mountpoint1,
				 &multithreaded1, NULL, data, 0);
	//fuse2 = setup_common(argc, argv2, &pfs_oper, sizeof(pfs_oper), &mountpoint2,
	//			 &multithreaded2, NULL, data2, 0);
	if (fuse1 == NULL)// || fuse2 == NULL)
		return 1;

	fprintf(stderr,"About to start fuse_loop1\n");
	if (multithreaded1)
		res1 = fuse_loop_mt(fuse1);
	else
		res1 = fuse_loop(fuse1);
/*	fprintf(stderr,"About to start fuse_loop2\n");
	if (multithreaded2)
		res2 = fuse_loop_mt(fuse2);
	else
		res2 = fuse_loop(fuse2);*/

	fuse_teardown(fuse1, mountpoint1);
	//fuse_teardown(fuse2, mountpoint2);
	if (res1 == -1)// || res2 == -1)
		return 1;

	return 0;
	/*char* test1[] = {"./pfs","../test1/"};
	char* test2[] = {"./pfs","../test2/"};
	char* test3[] = {"./pfs","../test3/"};
	fuse_stat = fuse_main(argc, argv, &pfs_oper, data);
	fprintf(stderr,"About to call fuse_main\n");
	fuse_stat = fuse_main(argc, test1, &pfs_oper, data);
	fprintf(stderr,"About to call fuse_main\n");
	fuse_stat = fuse_main(argc, test2, &pfs_oper, data);*/
	/*fprintf(stderr,"About to call fuse_main\n");
	int fuse_stat = fuse_main(argc, argv, &pfs_oper, data);*/
	//return fuse_stat;
}








