#include <stdint.h>
#include "nodes.h"

#define MAX_FILENAME 64
#define MAX_PATH 1024
#define DIRECTORY 2
#define NORMAL 1
#define ROOT 1

typedef struct
{
	char filename[MAX_FILENAME];
	uint64_t inum;
} dir;

uint64_t SBFS_open(char *filename, int mode);
int SBFS_close(int inum);
uint64_t SBFS_mkdir(char *path);
uint64_t SBFS_mknod(char *path);
int SBFS_rmdir(char* path);
uint64_t add_entry_to_dir(uint64_t inum,char* filename,uint64_t file_inode);

int SBFS_rmdir(char *path);
int SBFS_unlink(char *path);
dir *SBFS_readdir(uint64_t inum);
uint64_t SBFS_namei(char *path);
int SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf);
int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf);
void SBFS_init();