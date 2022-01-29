#include "disk.h"
#include <stdio.h>
#include <string.h>

//-1 for error 0 for success
int read_disk(uint64_t block_id, void *buffer)
{
	char *data = (char *)buffer;
	if (block_id >= BLOCKCOUNT)
	{
		printf("block read: %ld exceeded\n",block_id);
		return -1;
	}
	memcpy(data, dev[block_id], BLOCKSIZE);
	return 0;
}

int write_disk(uint64_t block_id, void *buffer)
{
	char *data = (char *)buffer;
	if (block_id >= BLOCKCOUNT)
	{
		printf("block write: %ld exceeded\n",block_id);
		return -1;
	}
	memcpy(dev[block_id], data, BLOCKSIZE);
	return 0;
}