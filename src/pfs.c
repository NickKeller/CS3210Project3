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
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>

int mapNameToDrives(const char* path){
	log_msg("Entered mapNameToDrives, path is: %s\n",path);
	char* key = calloc(strlen(path),sizeof(char));
	strcpy(key,path);
	struct node* drive = search(key);
	int driveNum = atoi(&drive->mount[strlen(drive->mount)-1]);
	log_msg("Drive Num:%d\n",driveNum);
	return driveNum;
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
	log_msg("Making dir: %s\n",path);
	pfs_fullpath(fpath, path);
	retstat = mkdir(fpath,mode);	
	
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = mkdir(fpath2,mode);
			if(res2 < 0){
				log_msg("ERROR: pfs_mkdir on backup/%s",drive);
				pfs_error("pfs_mkdir");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}	
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
	//backup
	if(PRI_DATA->master == 1){
		log_msg("Deleting image %s from database\n",fpath);
		int databaseRes = deleteImage(fpath);
		if(databaseRes == 0){
			log_msg("ERROR IN PUSHING TO DATABASE - deleteImage\n");
		}
		log_msg("Done deleting image %s from database\n",fpath);
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = unlink(fpath2);
			if(res2 < 0){
				log_msg("ERROR: pfs_unlink on backup/%s",drive);
				pfs_error("pfs_unlink");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = rmdir(fpath2);
			if(res2 < 0){
				log_msg("ERROR: pfs_rmdir on backup/%s",drive);
				pfs_error("pfs_rmdir");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
	//backup
	if(PRI_DATA->master == 1){
		//update database
		log_msg("Updating path: %s\nNewPath:%s\n",fpath,fnewpath);
		int databaseRes = updatePath(fnewpath,fpath);
		if (databaseRes == 0){
			log_msg("ERROR IN PUSHING TO DATABASE - updatePath\n");
		}
		log_msg("Done updating path\n");
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char fnewpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			//old path
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			//new path
			strcpy(fnewpath2,PRI_DATA->backup);
			strncat(fnewpath2,"/",1);
			strncat(fnewpath2,drive,strlen(drive));
			strncat(fnewpath2,newpath,strlen(newpath));
						
			log_msg("Writing %s\n to %s\n",fnewpath2,fpath2);
			int res2 = rename(fpath2,fnewpath2);
			if(res2 < 0){
				log_msg("ERROR: pfs_rename on backup/%s\n",drive);
				pfs_error("pfs_rename");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\nfnewpath2:%s\n",fpath2,fnewpath2);
		}
		
	}
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
 	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = chmod(fpath2,mode);
			if(res2 < 0){
				log_msg("ERROR: pfs_chmod on backup/%s\n",drive);
				pfs_error("pfs_chmod");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}   
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
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = chown(fpath2, uid, gid);
			if(res2 < 0){
				log_msg("ERROR: pfs_chown on backup/%s\n",drive);
				pfs_error("pfs_chown");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = truncate(fpath2, newsize);
			if(res2 < 0){
				log_msg("ERROR: pfs_truncate on backup/%s\n",drive);
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
			}
			drivesWrittenTo++;
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
	if(retstat < 0) retstat = pfs_error("pfs_truncate truncate");
	
	return retstat;
}

static int pfs_utime(const char* path, struct utimbuf *ubuf){
	log_msg("Entered pfs_utime\n");
	int retstat;
	char fpath[PATH_MAX];
	
	pfs_fullpath(fpath,path);
	
	retstat = utime(fpath,ubuf);
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = utime(fpath2,ubuf);
			if(res2 < 0){
				log_msg("ERROR: pfs_utime on backup/%s\n",drive);
				pfs_error("pfs_utime");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
	 //backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int fd = open(fpath2, fi->flags);
			int res2 = pwrite(fd,buf,size,offset);
			close(fd);
			if(res2 < 0){
				log_msg("ERROR: pfs_setxattr on backup/%s\n",drive);
				pfs_error("pfs_setxattr");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
    //backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = lsetxattr(fpath2, name, value, size, flags);
			if(res2 < 0){
				log_msg("ERROR: pfs_setxattr on backup/%s\n",drive);
				pfs_error("pfs_setxattr");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
    //backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = lremovexattr(fpath2, name);
			if(res2 < 0){
				log_msg("ERROR: pfs_removexattr on backup/%s\n",drive);
				pfs_error("pfs_removexattr");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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
	if(PRI_DATA->master == 1){
		for(int i = 0; i < PRI_DATA->numMounts; i++){
			char* total = calloc(strlen(PRI_DATA->backup) + 2,sizeof(char));
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",i);
			strcpy(total,PRI_DATA->backup);
			strncat(total,"/",1);
			strncat(total,drive,1);
			log_msg("\tFilepath is:%s\n",total);
			addNode(total);
			struct node* test = search("Key");
			int driveNum = atoi(&total[strlen(total)-1]);
			log_msg("\tMount from node is:%s\nSize is:%d\ndriveNum is:%d\n",test->mount,getSize(),driveNum);
		}
	}
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
	//write to master/node
	int retstat = 0;
	char fpath[PATH_MAX];
	int fd;
	
	pfs_fullpath(fpath,path);
	
	fd = creat(fpath, mode);
	//backup
	if(PRI_DATA->master == 1){
		log_msg("Pusing image %s to database\n",fpath);
		int databaseRes = insertImage(fpath);
		if(databaseRes == 0){
			log_msg("ERROR IN PUSHING TO DATABASE - insertImage\n");
		}
		log_msg("Done pushing image %s to database\n",fpath);
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = creat(fpath2, mode);
			if(res2 < 0){
				log_msg("ERROR: pfs_create on backup/%s\n",drive);
				pfs_error("pfs_create");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
	if(fd < 0) retstat = pfs_error("pfs_create creat");
	
	fi->fh = fd;
	
	return retstat;
}

static int pfs_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi){
	log_msg("Entered pfs_ftruncate\n");
	int retstat = 0;
	retstat = ftruncate(fi->fh, offset);
	//backup
	if(PRI_DATA->master == 1){
		int startDrive = mapNameToDrives(path);
		int drivesWrittenTo = 0;
		while(drivesWrittenTo < (PRI_DATA->numMounts - 2)){
			log_msg("Drives Written To:%d\nTrying to write to backup:%d\n",drivesWrittenTo,startDrive);
			char fpath2[PATH_MAX];
			char* drive = calloc(1,sizeof(char));
			sprintf(drive,"%d",startDrive);
			strcpy(fpath2,PRI_DATA->backup);
			strncat(fpath2,"/",1);
			strncat(fpath2,drive,strlen(drive));
			strncat(fpath2,path,strlen(path));
			log_msg("Writing to %s\n",fpath2);
			int res2 = truncate(fpath2,offset);
			if(res2 < 0){
				log_msg("ERROR: pfs_create on backup/%s\n",drive);
				pfs_error("pfs_create");
			}
			else{
				log_msg("Successful write to:%s\n",fpath2);
				drivesWrittenTo++;
			}
			
			startDrive = (startDrive+1) % PRI_DATA->numMounts;
			log_msg("fpath2:%s\n",fpath2);
		}
		
	}
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

int main(int argc, char *argv[])
{
	if(argc < 5 || argc == 6 || argc > 7){
		fprintf(stderr, "usage: pfs [-m numMounts] logfileName backupMaster backupDir mountPoint\n");
		return 0;
	}
	
	struct state* data = calloc(1,sizeof(struct state));
	
	if(argc == 7){
		data->master = 1;
		data->numMounts = atoi(argv[2]);
		printf("NumMounts:%d\n",data->numMounts);
	}
	else{
		data->master = 0;
		data->numMounts = 0;
	}
	
	char* args[2];
	args[0] = "./pfs";
	args[1] = argv[argc-1];
	
	data->rootdir = realpath(argv[argc-2], NULL);
	data->backup = realpath(argv[argc-3],NULL);
	data->logfile = log_open(argv[argc-4]);
	
	fprintf(stderr,"MountDir is: %s\n",args[1]);
	fprintf(stderr,"Rootdir is: %s\n",data->rootdir);
	fprintf(stderr,"Backup is: %s\n",data->backup);
	printf("Argc:%d\n",argc);
	for(int i = 0; i < 2; i++){
		printf("Args[%d]:%s\n",i,args[i]);
	}
	//exit(0);
	fprintf(stderr,"About to call fuse_main\n");
	return fuse_main(2,args,&pfs_oper,data);
	
}
