#include "disk.h"
#include <stdio.h>
#include <string.h>
#include "nodes.h"
#include <math.h>

#define INODES (BLOCKCOUNT/10)
#define INODE_PER_BLOCK (BLOCKSIZE/(sizeof(inode)/8))

void init_free_disk(int start){
	char data[BLOCKSIZE];
	int total_per_block = BLOCKSIZE / BLOCKADDR;
	int record_block_count = (BLOCKCOUNT - INODES - 1) / total_per_block;
	uint64_t* datablock = (uint64_t*) data;
	int block_id = start;

	head = start;

	while(record_block_count--){
		block_id = start;
		memset(data,0,BLOCKSIZE);
		for(int i = 1;i<total_per_block;i++){
			start ++;
			datablock[i] = start + 1;
		}
		start++;
		datablock[0] = start;
		write_disk(block_id,datablock);
	}

	if((BLOCKCOUNT - INODES - 1)% total_per_block !=0){
		int block_id = start;
		memset(data,0,BLOCKSIZE);
		for(int i = 1;i<total_per_block;i++){
			start ++;
			
			if(start>(BLOCKCOUNT-1)){
				datablock[i] = 0;
			}
			else{
				datablock[i] = start + 1;
			}
		}
		datablock[0] = 0;
		write_disk(block_id,datablock);
	}
	else{
		datablock[0] = 0;
		write_disk(block_id,datablock); //rewrite the last block
	}
}

int mkfs(){
	int i =  0;
	char tmp[BLOCKSIZE];
	memset(tmp,0,sizeof(tmp));
	uint64_t* supernode  = (uint64_t*) tmp;
	int start  = INODES/INODE_PER_BLOCK + 1;
	*supernode = start;
	write_disk(0,tmp);

	printf("data block starts from %d\n",start);
	init_free_disk(start);
}

// inum start from zero
inode get_inode(uint32_t inum){
	uint32_t block_id = 1 + inum / INODE_PER_BLOCK;
	uint32_t offset = sizeof(inode)*(inum%INODE_PER_BLOCK);
}

int allocate_inode(inode data){

}

int free_inode(inode data){

}

int allocate_data_block(){

}

int free_data_block(int id){

}

/* when to write back is a serious question */
int read_block(unsigned int block_id,unsigned int offset,unsigned int size,void* buffer){
	char tmp[BLOCKSIZE];

	if(offset>BLOCKSIZE){
		printf("offset %d in write_disk greate than BLOCKSIZE\n",offset);
		return -1;
	}

	read_disk(block_id,tmp);
	memcpy((char*)buffer+offset,tmp,fmin(size,BLOCKSIZE-offset));

	return 0;
}


/* if the actual address (size + offset) exceed what block can hold, then only write part */
int write_block(unsigned int block_id,unsigned int offset,unsigned int size,void* buffer){
	char tmp[BLOCKSIZE];

	if(offset>BLOCKSIZE){
		printf("offset %d in write_disk greate than BLOCKSIZE\n",offset);
		return -1;
	}

	read_disk(block_id,tmp);
	memcpy(tmp+offset,(char*)buffer,fmin(size,BLOCKSIZE-offset));

	write_disk(block_id,tmp);

	return 0;
}

int main(){
	char buffer[4096] = "hello world";
	char tmp[4096];
	write_disk(5,buffer);
	read_disk(5,tmp);
	printf("%s\n",tmp);
	printf("%ld\n",sizeof(inode));
	return 0;
}