#include "sbFuse.h"
#include <fuse.h>

#define FUSE_USE_VERSION 31

static const struct fuse_operations sb_oper = {
	.init = sb_init,
	.getattr = sb_getattr,
	.opendir = sb_opendir,
	.readdir = sb_readdir,
	.releasedir = sb_releasedir,
	.open = sb_open,
	.read = sb_read,
	.write = sb_write,
	.release = sb_release,
	.mkdir = sb_mkdir,
	.mknod = sb_mknod,
	.unlink = sb_unlink,
	.rmdir = sb_rmdir,

};

static void *sb_init(struct fuse_conn_info *conn,
					 struct fuse_config *cfg)
{
	printf("\nsb_init\n");
	(void)conn;
	(void)cfg;

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
		stbuf->st_size = node.size;
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
		printf("read dir failed.");
		return -1;
	}
	else
	{
		filler(buf, ".", NULL, 0, 0);
		filler(buf, "..", NULL, 0, 0);
		// read file in path 1 by 1
		while (1)
		{
			dir *dr = SBFS_readdir(inum);
			if (dr)
			{
				filler(buf, dr->filename, NULL, 0, 0);
			}
			else
			{
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
		printf("open dir failed.");
		return -1;
	}
	fi->fh = inum;
	inode node;
	read_inode(inum, &node);

	if (node.type != DIRECTORY)
	{
		printf("open dir failed.");
		return -1;
	}
	else
	{
		return 0;
	}
}

static int sb_releasedir(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_releasedir(path=\"%s\", fi=0x%08x)\n", path, fi);
	int ret = SBFS_close(fi->fh);
	return ret;
}

static int sb_mknod(const char *path, mode_t mode, dev_t dev)
{
	printf("\nsb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n", path, mode, dev);
	inode node;
	int ret = SBFS_mknod(path, &node);
	if (ret == 0)
	{
		printf("mknod failed");
		return -1;
	}
	return ret;
}

static int sb_mkdir(const char *path, mode_t mode)
{
	printf("\nsb_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
	inode node;
	int ret = SBFS_mkdir(path, &node);
	if (ret == 0)
	{
		printf("mkdir failed");
		return -1;
	}
	return ret;
}

static int sb_open(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_open(path\"%s\", fi=0x%08x)\n", path, fi);

	uint64_t inum = SBFS_open(path, 1);
	if (inum == 0)
	{
		printf("open failed");
		return -1;
	}
	fi->fh = inum;
	return inum;
}

static int sb_read(const char *path, char *buf, size_t size, off_t offset,
				   struct fuse_file_info *fi)
{
	printf("\nsb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);
	int ret = SBFS_read(fi->fh, offset, size, buf);
	return ret;
}

static int sb_write(const char *path, const char *buf, size_t size,
					off_t offset, struct fuse_file_info *fi)
{
	printf("\nsb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
		   path, buf, size, offset, fi);
	int ret = SBFS_write(fi->fh, offset, size, buf);
	return ret;
}

static int sb_release(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_release(path=\"%s\", fi=0x%08x)\n", path, fi);
	int ret = SBFS_close(fi->fh);
	return ret;
}

/** Remove a file */
static int sb_unlink(const char *path)
{
	printf("\nsb_unlink(path=\"%s\")\n", path);
	int ret = SBFS_unlink(path);
	return ret;
}

/** Remove a directory */
static int sb_rmdir(const char *path)
{
	printf("\nsb_rmdir(path=\"%s\")\n", path);
	int ret = SBFS_rmdir(path);
	return ret;
}

int main(int argc, char *argv[])
{
  	return fuse_main(argc, argv, &sb_oper, NULL);
}