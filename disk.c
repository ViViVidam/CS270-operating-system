#include "disk.h"
#include <stdio.h>
#include <string.h>

//-1 for error 0 for success
int read_disk(unsigned int block_id, void* buffer){
	char* data = (char* ) buffer;
	if(block_id>=BLOCKCOUNT){
		printf("block read: %d exceeded\n");
		return -1;
	}
	memcpy(data,dev[block_id],BLOCKSIZE);
	return 0;
}

int write_disk(unsigned int block_id, void* buffer){
	char* data = (char* ) buffer;
	if(block_id>=BLOCKCOUNT){
		printf("block write: %d exceeded\n");
		return -1;
	}
	memcpy(dev[block_id],data,BLOCKSIZE);
	return 0;
}

/*
int read_disk(unsigned int block_id,unsigned int offset,unsigned int size,char* buffer){
	int block_count = (size+offset)/BLOCKSIZE;

	if(offset>BLOCKSIZE){
		printf("offset %d in read_disk greate than BLOCKSIZE\n",offset);
		return -1;
	}

	if((size+offset)%BLOCKSIZE!=0){
		block_count += 1;
	}

	int write_size = fmin(size,BLOCKSIZE-offset);
	memcpy(buffer,dev[block_id]+offset,fmin(size,BLOCKSIZE-offset));

	buffer += write_size;
	size -= write_size;

	for(int i=1;i<block_count;i++){
		int tmp = fmin(size,BLOCKSIZE);
		memcpy(buffer,dev[i+block_id],tmp);
		buffer += tmp;
		size -= write_size;
	}

	return 0;
}

int write_disk(unsigned int block_id,unsigned int offset,unsigned int size,char* buffer){
	int block_count = (size+offset)/BLOCKSIZE;

	if(offset>BLOCKSIZE){
		printf("offset %d in write_disk greate than BLOCKSIZE\n",offset);
		return -1;
	}

	if((size+offset)%BLOCKSIZE!=0){
		block_count += 1;
	}

	int write_size = fmin(size,BLOCKSIZE-offset);
	memcpy(dev[block_id]+offset,buffer,fmin(size,BLOCKSIZE-offset));

	buffer += write_size;
	size -= write_size;

	for(int i=1;i<block_count;i++){
		int tmp = fmin(size,BLOCKSIZE);
		memcpy(dev[i+block_id],buffer,tmp);
		buffer += tmp;
		size -= write_size;
	}

	return 0;
}*/