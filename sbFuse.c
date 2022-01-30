#include "sbFuse.h"
#include "libfuse/include/fuse.h"

#define FUSE_USE_VERSION 31

static const struct fuse_operations hello_oper = {
	.init           = sb_init,
	.getattr	= sb_getattr,
	.readdir	= sb_readdir,
	.open		= sb_open,
	.read		= sb_read,
};

static void *sb_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;

	return NULL;
}
