#include "disk.h"
#include <stdio.h>
#include <string.h>
#include "nodes.h"
#include <math.h>
#include <assert.h>

#define INODE_PER_BLOCK (BLOCKSIZE / (sizeof(inode) / 8))

void cp_inode_length(inode *dest, inode *source)
{
	uint64_t *dest_alter = (uint64_t *)dest;
	uint64_t *source_alter = (uint64_t *)source;
	for (int i = 0; i < (DIRECT_BLOCK + SING_INDIR + DOUB_INDIR + TRIP_INDIR); i++)
	{
		dest_alter[i] = source_alter[i];
	}
	dest->flag = source->flag;
	dest->type = source->type;
	dest->size = source->size;
}

void write_inode(uint64_t inum, inode *node)
{
	char data[BLOCKSIZE];
	uint64_t block_id = inum / INODE_PER_BLOCK + 1;
	uint64_t offset = inum % INODE_PER_BLOCK;
	read_disk(block_id, data);
	inode *inodes = (inode *)data;

	/* length is used to escape zero out the memory when allocate inode in user space */
	cp_inode(&inodes[offset], node, length);
	write_disk(block_id, data);
}

void init_free_disk(int start)
{
	char data[BLOCKSIZE];
	int total_per_block = BLOCKSIZE / BLOCKADDR;
	int record_block_count = (BLOCKCOUNT - INODES - 1) / total_per_block;
	uint64_t *datablock = (uint64_t *)data;
	int block_id = start;

	head = start;

	while (record_block_count--)
	{
		block_id = start;
		memset(data, 0, BLOCKSIZE);
		for (int i = 1; i < total_per_block; i++)
		{
			start++;
			datablock[i] = start + 1;
		}
		start++;
		datablock[0] = start;
		write_disk(block_id, datablock);
	}

	if ((BLOCKCOUNT - INODES - 1) % total_per_block != 0)
	{
		int block_id = start;
		memset(data, 0, BLOCKSIZE);
		for (int i = 1; i < total_per_block; i++)
		{
			start++;

			if (start > (BLOCKCOUNT - 1))
			{
				datablock[i] = 0;
			}
			else
			{
				datablock[i] = start + 1;
			}
		}
		datablock[0] = 0;
		write_disk(block_id, datablock);
	}
	else
	{
		datablock[0] = 0;
		write_disk(block_id, datablock); //rewrite the last block
	}
}

void init_i_list()
{
	char tmp[BLOCKSIZE];
	memset(tmp, 0, sizeof(tmp));
	i_list_block_count = BLOCKCOUNT / 10;
	for (int i = 1; i <= i_list_block_count; i++)
	{
		write_block(i, 0, BLOCKSIZE, tmp);
	}
}

int mkfs()
{
	int i = 0;
	char tmp[BLOCKSIZE];

	memset(tmp, 0, sizeof(tmp));

	init_i_list();

	uint64_t *supernode = (uint64_t *)tmp;

	//int start = INODES / INODE_PER_BLOCK + 1;
	int start = i_list_block_count + 1;
	//start of block
	supernode[0] = start;
	supernode[1] = i_list_block_count; //inode block count 
	//supernode[1] = i_list_block_count * BLOCKSIZE / ((sizeof(inode)/8)); //inode count 
	supernode[2] = BLOCKSIZE;
	supernode[3] = 1; //init flag
	write_disk(0, tmp);

	printf("data block starts from block %d\n", start);
	init_free_disk(start);
}

/* 
	inum start from zero
	I don't think it needs to be reading a length
*/
void read_inode(uint64_t inum, inode *node)
{
	char data[BLOCKSIZE];
	uint32_t block_id = 1 + inum / INODE_PER_BLOCK;
	uint32_t offset = inum % INODE_PER_BLOCK;
	inode node_tmp;
	read_disk(block_id, data);
	inode *inodes = (*inode)data;
	cp_inode(node, &inodes[offset], DIRECT_BLOCK + SING_INDIR + DOUB_INDIR + TRIP_INDIR);
}

/* zero for failure */
uint64_t allocate_inode()
{
	char tmp[BLOCKSIZE];
	int i, j, flag, block_id;
	inode *inode_block = (*uint64_t)tmp;
	for (i = 1; i <= i_list_block_count; i++)
	{
		read_disk(i, 0, BLOCKSIZE, tmp);
		for (j = 0; j < INODE_PER_BLOCK; j++)
		{
			if (inode_block[j].flag == 0)
			{
				flag = 1;
				break;
			}
		}
	}
	if (flag)
	{
		block_id = (i - 1) * INODE_PER_BLOCK + j;
		inode_block[j].flag = 1;
		write_disk(i, 0, BLOCKSIZE, tmp);
	}
	else
		block_id = -1;
	return block_id;
}

int free_inode(uint64_t inum)
{
	uint64_t block_id = 1 + inum / INODE_PER_BLOCK;
	uint32_t offset = inum % INODE_PER_BLOCK;
	inode tmp;
	if (block_id > i_list_block_count)
	{
		printf("block_id too big %d \n", inum);
		return -1;
	}
	read_block(block_id, sizeof(inode) * offset, sizeof(inode), &tmp);

	for (int i = 0; i < DIRECT_BLOCK; i++)
	{
		free_data_block(tmp.direct_blocks[i]);
		tmp.direct_blocks[i] = 0;
	}
	for (int i = 0; i < SING_INDIR; i++)
	{
		free_data_block(tmp.sing_indirect_blocks[i] = 0);
		tmp.sing_indirect_blocks[i] = 0;
	}
	for (int i = 0; i < DOUB_INDIR; i++)
	{
		free_data_block(tmp.doub_indirect_blocks[i] = 0);
		tmp.doub_indirect_blocks[i] = 0;
	}
	for (int i = 0; i < TRIP_INDIR; i++)
	{
		free_data_block(tmp.trip_indirect_blocks[i] = 0);
		tmp.trip_indirect_blocks[i] = 0;
	}
	tmp.flag = 0;
	tmp.size = 0;
	tmp.type = 0;
	write_block(block_id, sizeof(inode) * offset, sizeof(inode), &tmp);

	return 0;
}

uint64_t allocate_data_block()
{
	uint8_t tmp[BLOCKSIZE];
	uint64_t *data = (uint64_t *)tmp;
	read_block(head, 0, BLOCKSIZE, tmp);
	int i = 0;
	int res = 0;
	for (i = 1; i < (BLOCKSIZE / 4); i++)
	{
		if (data[i] != 0)
		{
			res = 1;
			break;
		}
	}
	if (res == 1)
	{
		res = data[i];
		data[i] = 0;
		write_disk(head, 0, BLOCKSIZE, tmp);
	}
	else
	{
		res = head;
		head = data[0];
		data[0] = 0;
		write_disk(0, 0, sizeof(uint64_t), &head);
		write_disk(res, 0, BLOCKSIZE, tmp);
	}
	return res;
}

int free_data_block(uint64_t id)
{
	uint8_t tmp[BLOCKSIZE];
	uint64_t *data = (uint64_t *)tmp;
	read_block(head, 0, BLOCKSIZE, tmp);
	int i = 0, flag = 0;
	for (i = 1; i < (BLOCKSIZE / 4); i++)
	{
		if (data[i] == 0)
		{
			flag = 1;
			data[i] = id;
			break;
		}
	}

	if (flag == 1)
	{
		write_block(head, 0, BLOCKSIZE, tmp);
	}
	else
	{
		int pre_head = head;
		head = id;
		write_block(head, 0, sizeof(uint64_t), &pre_head);
	}
	return 0;
}

/* when to write back is a serious question */
int read_block(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
	char tmp[BLOCKSIZE];

	if (offset > BLOCKSIZE)
	{
		printf("offset %d in write_disk greate than BLOCKSIZE\n", offset);
		return -1;
	}

	read_disk(block_id, tmp);
	memcpy((char *)buffer, tmp + offset, fmin(size, BLOCKSIZE - offset));

	return fmin(size, BLOCKSIZE - offset);
}

int write_block(uint64_t block_id,uint64_t offset,uint64_t size,void* buffer)
{
	char tmp[BLOCKSIZE];

	if (offset > BLOCKSIZE)
	{
		printf("boundary exceeded %d in write_disk greate than BLOCKSIZE\n", offset);
		return -1;
	}

	read_disk(block_id, tmp);
	memcpy(tmp + offset, (char *)buffer, fmin(size, BLOCKSIZE - offset));

	write_disk(block_id, tmp);

	return fmin(size, BLOCKSIZE - offset);
}

int main()
{
	char buffer[4096] = "hello world";
	char tmp[4096];
	write_disk(5, buffer);
	read_disk(5, tmp);
	printf("%s\n", tmp);
	printf("%ld\n", sizeof(inode));
	return 0;
}
/* size is for computation convinent */
/*void write_block(uint64_t inum,uint64_t index,void* buf,uint64_t size){
	inode node;
	read_inode(inum,&node);
	char tmp[BLOCKSIZE];
	char zero[BLOCKSIZE];
	memset(zero,0,BLOCKSIZE);
	uint64_t* address = (uint64_t*)tmp;
	int flag = 0;// indicate is there a change in inode
	if(index<DIRECT_BLOCK){
		if(node.direct_blocks[index]==0){
			node.direct_blocks[index] = allocate_data_block();
		}
		write_disk(node.direct_blocks[index],0,BLOCKSIZE,buf);
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512) ){
		if(node.sing_indirect_blocks[0]==0){
			node.sing_indirect_blocks[0] = allocate_data_block();
			write_disk(node.sing_indirect_blocks[0],zero);
		}
		// I/O can be cut down, but I will leave it for now
		read_disk(node->sing_indirect_blocks[0],0,BLOCKSIZE,tmp);
		if(node.sing_indirect_blocks[0]==0){
			node.sing_indirect_blocks = allocate_data_block();
		}
		write_disk(address[index-DIRECT_BLOCK],0,BLOCKSIZE,buf);
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512+DOUB_INDIR*512*512)){
		if(node.doub_indirect_blocks[0]==0){
			node.doub_indirect_blocks[0] = allocate_data_block();
			write_disk(node.doub_indirect_blocks[0],zero);
		}
		read_disk(node.doub_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/512;
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_disk(address[next_level_index],zero);
		}
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_disk(address[next_level_index],zero);
		}
		index = (index - DIRECT_BLOCK - SING_INDIR*512)%512;
		if(address[index]==0){
			address[index] = allocate_data_block();
		}
		write_disk(address[index],0,BLOCKSIZE,buf);
	}
	else{
		if(node.trip_indirect_blocks[0]==0){
			node.trip_indirect_blocks[0] = allocate_data_block();
			write_disk(node.trip_indirect_blocks[0],zero);
		}
		read_disk(node.trip_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ (512*512);
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_disk(address[next_level_index],zero);
		}
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ 512;
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_disk(address[next_level_index],zero);
		}
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR*512)%512;
		if(address[index]==0){
			address[index] = allocate_data_block();
		}
		write_disk(address[index],0,BLOCKSIZE,buf);
	}
	node->size += size;
	write_inode(inum,&node);
}*/

/* need robust */
/*int read_block(uint64_t inum,uint64_t index,void* buf){
	inode node;
	read_inode(inum,&node);
	char tmp[BLOCKSIZE];
	uint64_t* address = (uint64_t*)tmp;
	if(index<DIRECT_BLOCK){
		if(node.direct_blocks[index]==0)
			return 0;
		read_disk(node.direct_blocks[index],0,BLOCKSIZE,buf);
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512) ){
		if(node.sing_indirect_blocks[0]==0)
			return 0;
		read_disk(node.sing_indirect_blocks[0],0,BLOCKSIZE,tmp);
		if(address[index-DIRECT_BLOCK]==0)
			return 0;
		read_disk(address[index-DIRECT_BLOCK],0,BLOCKSIZE,buf);
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512+DOUB_INDIR*512*512)){
		if(node.doub_indirect_blocks[0]==0)
			return 0;
		read_disk(node.doub_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/512;
		if(address[next_level_index]==0)
			return 0;
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR*512)%512;
		if(address[index]==0)
			return 0;
		read_disk(address[index],0,BLOCKSIZE,buf);
	}
	else{
		if(node.trip_indirect_blocks[0]==0)
			return 0;
		read_disk(node.trip_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ (512*512);
		if(address[next_level_index]==0)
			return 0;
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ 512;
		if(address[next_level_index]==0)
			return 0;
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR*512)%512;
		if(address[index]==0)
			return 0;
		read_disk(address[index],0,BLOCKSIZE,buf);
	}
	return 1;
}*/
/* if the actual address (size + offset) exceed what block can hold, then only write part */
