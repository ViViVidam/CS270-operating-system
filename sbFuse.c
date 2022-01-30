#include "sbFuse.h"
#include "libfuse/include/fuse.h"

#define FUSE_USE_VERSION 31

static const struct fuse_operations hello_oper = {
	.init = sb_init,
	.getattr = sb_getattr,
	.readdir = sb_readdir,
	.open = sb_open,
	.read = sb_read,
	.mkdir = sb_mkdir,
	.mknod = sb_mknod,
};

static void *sb_init(struct fuse_conn_info *conn,
					 struct fuse_config *cfg)
{
	(void)conn;

	SBFS_init();

	return NULL;
}

static int sb_getattr(const char *path, struct stat *stbuf,
					  struct fuse_file_info *fi)
{
	printf("\nsb_getattr(path=\"%s\", stbuf=0x%08x, fi = 0x%08x)\n", path, stbuf, fi);

	memset(stbuf, 0, sizeof(struct stat));

	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	stbuf->st_atime = time(NULL);
	stbuf->st_mtime = time(NULL);

	uint64_t inum = SBFS_namei(path);
	fi->fh = inum;
	inode node;
	read_inode(inum, &node);

	if (node.type == DIRECTORY)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		stbuf->st_size = node.size;
	}

	return 0;
}

static int sb_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
					  off_t offset, struct fuse_file_info *fi,
					  enum fuse_readdir_flags flags)
{
	printf("\nsb_readdir(path=\"%s\", fi = 0x%08x)\n", path, fi);

	(void)offset;
	//(void)fi;
	(void)flags;

	uint64_t inum = fi->fh;
	inode node;
	read_inode(inum, &node);

	if (node.type != DIRECTORY)
	{
		return -1;
	}
	else
	{
		filler(buf, ".", NULL, 0, 0);
		filler(buf, "..", NULL, 0, 0);
		// read file in path 1 by 1
		while (1)
		{
			dir* dr = SBFS_readdir(inum);
			if(dr) {
				filler(buf, dr->filename, NULL, 0, 0);
			} else {
				break;
			}
		}
	}

	return 0;
}

static int sb_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);
	uint64_t inum = SBFS_namei(path);

	//wrong inum
	if (inum == 0)
	{
		return -1;
	}
	fi->fh = inum;
	inode node;
	read_inode(inum, &node);

	if (node.type != DIRECTORY)
	{
		return -1;
	}
	else
	{
		// how to open? are there anything else?
		return 0;
	}
}

static int sb_mknod(const char *path, mode_t mode, dev_t dev) {

}

static int sb_mkdir (const char *path, mode_t mode) {

}


static int sb_open(const char *path, struct fuse_file_info *fi)
{

	return 0;
}

static int sb_read(const char *path, char *buf, size_t size, off_t offset,
				   struct fuse_file_info *fi)
{
	size_t len;
	(void)fi;

	return size;
}