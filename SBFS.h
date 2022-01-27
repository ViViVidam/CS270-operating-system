#define MAX_FILENAME 64
int SBFS_open(char*,int flag);
int SBFS_close(int fd);
int SBFS_mkdir();
int SBFS_mknod();
int SBFS_unlink();
int SBFS_read();
int SBFS_write();
int SBFS_init();