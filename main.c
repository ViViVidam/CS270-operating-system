#include <errno.h>
#include <fuse.h>
#include <string.h>

static int
fs_readdir(const char *path, void *data, fuse_fill_dir_t filler,
           off_t off, struct fuse_file_info *ffi)
{
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(data, ".", NULL, 0);
	filler(data, "..", NULL, 0);
	filler(data, "file", NULL, 0);
	return 0;
}

static int
fs_read(const char *path, char *buf, size_t size, off_t off,
        struct fuse_file_info *ffi)
{
	size_t len;
	const char *file_contents = "fuse filesystem example\n";

	len = strlen(file_contents);

	if (off < len) {
		if (off + size > len)
			size = len - off;
		memcpy(buf, file_contents + off, size);
	} else
		size = 0;

	return size;
}

static int
fs_open(const char *path, struct fuse_file_info *ffi)
{
	if (strncmp(path, "/file", 10) != 0)
		return -ENOENT;

	if ((ffi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int
fs_getattr(const char *path, struct stat *st)
{
	if (strcmp(path, "/") == 0) {
		st->st_blksize = 512;
		st->st_mode = 0755;
		st->st_nlink = 2;
	} else if (strcmp(path, "/file") == 0) {
		st->st_mode = 0644;
		st->st_blksize = 512;
		st->st_nlink = 1;
		st->st_size = 5;
	} else {
		return -ENOENT;
	}

	return 0;
}

/*
struct fuse_operations {
    int (getattr) (const char , struct stat );
    int (readlink) (const char , char , size_t);
    int (getdir) (const char , fuse_dirh_t, fuse_dirfil_t);
    int (mknod) (const char , mode_t, dev_t);
    int (mkdir) (const char , mode_t);
    int (unlink) (const char );
    int (rmdir) (const char );
    int (symlink) (const char , const char );
    int (rename) (const char , const char );
    int (link) (const char , const char );
    int (chmod) (const char , mode_t);
    int (chown) (const char , uid_t, gid_t);
    int (truncate) (const char , off_t);
    int (utime) (const char , struct utimbuf );
    int (open) (const char , struct fuse_file_info );
    int (read) (const char , char , size_t, off_t, struct fuse_file_info );
    int (write) (const char , const char , size_t, off_t,struct fuse_file_info );
    int (statfs) (const char , struct statfs );
    int (flush) (const char , struct fuse_file_info );
    int (release) (const char , struct fuse_file_info );
    int (fsync) (const char , int, struct fuse_file_info );
    int (setxattr) (const char , const char , const char , size_t, int);
    int (getxattr) (const char , const char , char , size_t);
    int (listxattr) (const char , char , size_t);
    int (removexattr) (const char , const char *);
};*/

struct fuse_operations fsops = {
	//.readdir = fs_readdir,
	.read = fs_read,
	.open = fs_open,
	.getattr = fs_getattr,
};

int
main(int argc, char **argv)
{
	return (fuse_main(argc, argv, &fsops));
}