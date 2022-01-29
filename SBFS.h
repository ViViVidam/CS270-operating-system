#include <stdint.h>
#define MAX_FILENAME 64
#define EOF -1;
typedef struct
{
	char filename[MAX_FILENAME];
	uint64_t inum;
	//uint8_t valid;
} dir;

int SBFS_open(char *, int flag);
int SBFS_close(int fd);
int SBFS_mkdir();
int SBFS_mknod();
int SBFS_unlink();
int SBFS_read(char *filename, uint64_t offset, int64_t size, void *buf);
int SBFS_write(char *filename, uint64_t offset, int64_t size, void *buf);
int SBFS_init();