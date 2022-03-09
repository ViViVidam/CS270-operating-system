#include "disk.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
static char volume_paths[128];
/**
 * this should check if path length is over 128
 * but we skip this
 * **/
void set_vol(const char* path){
    strcpy(volume_paths,path);
}

//-1 for error 0 for success
int read_disk(uint64_t block_id, void *buffer)
{
    buffer[BLOCKSIZE];
    int fp = open(volume_paths,O_RDONLY);
    if (block_id >= BLOCKCOUNT)
    {
        printf("block read: %ld exceeded\n",block_id);
        return -1;
    }
    lseek(fp,BLOCKSIZE*block_id,SEEK_SET);
    read(fp,buffer, BLOCKSIZE);
    close(fp);
    return 0;
}

int write_disk(uint64_t block_id, void *buffer)
{
    int fp = open(volume_paths,O_WRONLY);
	if (block_id >= BLOCKCOUNT)
	{
		printf("block write: %ld exceeded\n",block_id);
		return -1;
	}
    lseek(fp,BLOCKSIZE*block_id,SEEK_SET);
    write(fp,buffer, BLOCKSIZE);
    close(fp);
	return 0;
}