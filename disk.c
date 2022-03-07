#include "disk.h"
#include <stdio.h>
#include <string.h>

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
    FILE*fp = fopen(volume_paths,"r");
	if (block_id >= BLOCKCOUNT)
	{
		printf("block read: %ld exceeded\n",block_id);
		return -1;
	}
    fseek(fp,BLOCKSIZE*block_id,SEEK_SET);
    fread(buffer,sizeof(uint8_t),BLOCKSIZE,fp);
    fclose(fp);
	return 0;
}

int write_disk(uint64_t block_id, void *buffer)
{
    FILE*fp = fopen(volume_paths,"w+");
	if (block_id >= BLOCKCOUNT)
	{
		printf("block write: %ld exceeded\n",block_id);
		return -1;
	}
    fseek(fp,BLOCKSIZE*block_id,SEEK_SET);
    fwrite(buffer,sizeof(uint8_t),BLOCKSIZE,fp);
    fclose(fp);
	return 0;
}