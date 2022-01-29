#include <stdint.h>

#define MAX_FILENAME 64
#define DIRECTORY 2
#define NORMAL 1

typedef struct{
	char filename[MAX_FILENAME];
	uint64_t inum;
} dir;

uint64_t SBFS_open(char *filename, int mode);
int SBFS_close(int node);
uint64_t SBFS_mkdir(char *filename, inode *node);
uint64_t SBFS_mknod(char *filename, inode *node);
int SBFS_unlink(char *path);
dir *SBFS_readdir(uint64_t inum);
uint64_t SBFS_namei(char *path);
int SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf);
int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf);
void SBFS_init();