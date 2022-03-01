#ifndef CS270_SBFS_H
#define CS270_SBFS_H

#include <stdint.h>
#include "SBFSStruct.h"
#include "SBFSHelper.h"
#include <sys/stat.h>

uint64_t SBFS_open(char *filename, int mode);
int SBFS_close(int inum);
uint64_t SBFS_mkdir(char *path);
uint64_t SBFS_mknod(char *path);
unsigned long SBFS_readlink(char* path, char* buf);
int SBFS_rmdir(char *path);
int SBFS_unlink(char *path);
dir *SBFS_readdir(uint64_t inum,int init);
uint64_t SBFS_namei(char *path);
uint64_t SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf);
int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf);
void SBFS_init();
int SBFS_getattr(char* filename,struct stat* file_state);
#endif