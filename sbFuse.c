#define FUSE_USE_VERSION 31

#include "SBFS.h"
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>

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
	if (inum == 0)
		return -ENOENT;
	else
		printf("inum %ld\n", inum);
	//fi->fh = inum;
	inode node;
	read_inode(inum, &node);
	printf("read node inum %ld\n", inum);
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
    printf("npde size %ld %ld\n",inum,node.size);
	if (node.type != DIRECTORY)
	{
		printf("read dir failed.\n");
		return -1;
	}
	else
	{
		printf("add . to filler\n");
		filler(buf, ".", NULL, 0, 0);

		printf("add .. to filler\n");
		filler(buf, "..", NULL, 0, 0);
		// read file in path 1 by 1
		while (1)
		{	
			printf("call readdir on inum %ld\n", inum);
			dir *dr = SBFS_readdir(inum,0);
			printf("get dir pointer: 0x%08x\n", dr);
			if (dr)
			{
				printf("add filename: %s to filler\n", dr->filename);
				filler(buf, dr->filename, NULL, 0, 0);
			}
			else
			{
                printf("dir null\n");
				break;
			}
		}
	}
    SBFS_readdir(0,1);
	return 0;
}

static int sb_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);
	uint64_t inum = SBFS_namei(path);

	//wrong inum
	if (inum == 0)
	{
		printf("open dir failed.\n");
		return -1;
	}
	printf("set fi->fh = inum: %ld\n", inum);
	fi->fh = inum;
	// inode node;
	// read_inode(inum, &node);

	// if (node.type != DIRECTORY)
	// {
	// 	printf("open dir failed.\n");
	// 	return -1;
	// }
	// else
	// {
	return 0;
	// }
}

static int sb_releasedir(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_releasedir(path=\"%s\", fi=0x%08x)\n", path, fi);
	int ret = SBFS_close(fi->fh);
	return 0;
}

static int sb_mknod(const char *path, mode_t mode, dev_t dev)
{
	printf("\nsb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n", path, mode, dev);
	int ret = SBFS_mknod(path);
	if (ret == 0)
	{
		printf("mknod failed\n");
		return -1;
	}
	else
	{
		printf("mknod successed\n");
	}
	return 0;
}

static int sb_mkdir(const char *path, mode_t mode)
{
	printf("\nsb_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
	int ret = SBFS_mkdir(path);
	if (ret == 0)
	{
		printf("mkdir failed\n");
		return -1;
	}
	else
	{
		printf("mkdir successed\n");
	}
	return 0;
}

static int sb_open(const char *path, struct fuse_file_info *fi)
{
	printf("\nsb_open(path\"%s\", fi=0x%08x)\n", path, fi);

	uint64_t inum = SBFS_open(path, 1);
	if (inum == 0)
	{
		printf("open failed\n");
		return -1;
	}
	printf("set fi->fh = inum: %ld\n", inum);
	fi->fh = inum;
	return 0;
}

static int sb_read(const char *path, char *buf, size_t size, off_t offset,
				   struct fuse_file_info *fi)
{
	printf("\nsb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);
	int ret = SBFS_read(fi->fh, offset, size, buf);
	printf("read into buf: %s\n", buf);
	return 0;
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
	return 0;
}

/** Remove a file */
static int sb_unlink(const char *path)
{
	printf("\nsb_unlink(path=\"%s\")\n", path);
	int ret = SBFS_unlink(path);
	return 0;
}
/** Remove a directory */
static int sb_rmdir(const char *path)
{
	printf("\nsb_rmdir(path=\"%s\")\n", path);
	int ret = SBFS_rmdir(path);
	return 0;
}

static int sb_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi)
{
	return 0;
}

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
	.utimens = sb_utimens,

};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &sb_oper, NULL);
}