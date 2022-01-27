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
	supernode += sizeof(uint64_t);
	*supernode = 0; //inode count
	write_disk(0,tmp);

	printf("data block starts from %d\n",start);
	init_free_disk(start);
}

// inum start from zero
inode* get_inode(uint64_t inum){
	//uint64_t inodes_count = 0;
	//read_block(0,sizeof(uint64_t),sizeof(uint64_t),&inodes_count);
	/*if(inodes_count<inum){
		printf("get_inode inum wrong %d over %d",inum,inodes_count);
		return NULL;
	}*/
	uint32_t block_id = 1 + inum / INODE_PER_BLOCK;
	uint32_t offset = sizeof(inode)*(inum%INODE_PER_BLOCK);
	inode node_tmp;
	read_block(block_id,offset,sizeof(inode),&node_tmp);
	return &node_tmp
}

/* zero for failure */
uint64_t allocate_inode(inode* data){
	/*uint64_t inodes_count = 0;
	read_block(0,sizeof(uint64_t),sizeof(uint64_t),&inodes_count);
	if(inodes_count>INODES){
		printf("NO inode available %d over %d",inodes_count,INODES);
		return 0;
	}
	inodes_count += 1;*/
	uint32_t block_id = 1 + inodes_count / INODE_PER_BLOCK;
	uint32_t offset = sizeof(inode)*(inodes_count%INODE_PER_BLOCK);
	write_disk(block_id,offset,sizeof(inode),data);
	write_disk(0,sizeof(uint64_t),sizeof(uint64_t),&inodes_count);
	return inodes_count;
}

int free_inode(uint64_t inum){

}

uint64_t allocate_data_block(){
	uint8_t tmp[BLOCKSIZE];
	uint64_t* data = (uint64_t*) tmp;
	read_block(head,0,BLOCKSIZE,tmp);
	int i = 0;
	int res = 0;
	for(i=1;i<(BLOCKSIZE/4);i++){
		if(data[i]!=0){
			res = 1;
			break;
		}
	}
	if(res==1){
		res = data[i];
		data[i] = 0;
		write_disk(head,0,BLOCKSIZE,tmp);
	}
	else{
		res = head;
		head = data[0];
		data[0] = 0;
		write_disk(0,0,sizeof(uint64_t),&head);
		write_disk(res,0,BLOCKSIZE,tmp);
	}
	return res;
}

int free_data_block(int id){
	uint8_t tmp[BLOCKSIZE];
	uint64_t* data = (uint64_t*) tmp;
	read_block(head,0,BLOCKSIZE,tmp);
	int i = 0, flag = 0;
	for(i=1;i<(BLOCKSIZE/4);i++){
		if(data[i] == 0){
			flag = 1;
			data[i] = id;
			break;
		}
	}

	if(flag==1){
		write_block(head,0,BLOCKSIZE,tmp);
	}
	else{
		int pre_head = head;
		head = id;
		write_block(head,0,sizeof(uint64_t),&pre_head);
	}
	return 0;
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