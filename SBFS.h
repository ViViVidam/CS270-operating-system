#ifndef CS270_SBFS_H
#define CS270_SBFS_H

#include <stdint.h>
#include "SBFSStruct.h"
#include "SBFSHelper.h"
#include <sys/stat.h>

uint64_t SBFS_open(char *filename, unsigned int userId,unsigned int groupId,unsigned int flag);
int SBFS_close(int inum);
uint64_t SBFS_mkdir(char *path,unsigned int userId,unsigned int groupId);
uint64_t SBFS_mknod(char *path,unsigned int userId,unsigned int groupId);
int SBFS_readlink(char* path, char* buf, unsigned long size);
int SBFS_utime(char* path,const struct timespec tv[2]);
int SBFS_chown(char* path,uint16_t uid,uint16_t gid);
int SBFS_rename(char* path,char* newname,unsigned int flags);
int SBFS_chmod(char* filename,uint16_t mode);
int SBFS_link(char* path, char* newpath);
int SBFS_symlink(char* path,char* filename);
int SBFS_truncate(char* path, uint64_t newsize);
int SBFS_rmdir(char *path);
int SBFS_unlink(char *path);
dir *SBFS_readdir(uint64_t inum,int init);
uint64_t SBFS_namei(char *path);
uint64_t SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf);
int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf);
void SBFS_init();
int SBFS_getattr(char* filename,struct stat* file_state);
char mountpoint[256];
#endif