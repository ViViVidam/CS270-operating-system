#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "nodes.h"

#define INODE_PER_BLOCK (BLOCKSIZE / sizeof(inode))

static uint64_t head = 0;
static uint64_t i_list_block_count = 0;
static inode* in_mem_inode;

uint64_t read_head(){
    return head;
}

void cp_inode(inode *dest, inode *source)
{
	for (int i = 0; i < (DIRECT_BLOCK);i++){
		dest->direct_blocks[i] = source->direct_blocks[i];
	}
    for (int i = 0; i < (SING_INDIR);i++){
        dest->sing_indirect_blocks[i] = source->sing_indirect_blocks[i];
    }
    for (int i = 0; i < (DOUB_INDIR);i++){
        dest->doub_indirect_blocks[i] = source->doub_indirect_blocks[i];
    }
    for (int i = 0; i < (TRIP_INDIR);i++){
        dest->trip_indirect_blocks[i] = source->trip_indirect_blocks[i];
    }
	dest->flag = source->flag;
	dest->type = source->type;
	dest->size = source->size;
}
/*
 * not used anymore
 * */

void write_inode_old(uint64_t inum, inode *node)
{
	char data[BLOCKSIZE];
    //printf("write %ld %ld %ld\n",node->size,node->sing_indirect_blocks[0],node->flag);
	uint64_t block_id = (inum - 1) / INODE_PER_BLOCK + 1;
	uint64_t offset = (inum - 1) % INODE_PER_BLOCK;
	read_disk(block_id, data);
	inode *inodes = (inode *)data;

	/* length is used to escape zero out the memory when allocate inode in user space */
	cp_inode(&inodes[offset], node);
	write_disk(block_id, data);
}

void write_inode(uint64_t inum, inode *node)
{
    char data[BLOCKSIZE];
    //printf("write %ld %ld %ld\n",node->size,node->sing_indirect_blocks[0],node->flag);
    uint64_t block_id = (inum - 1) / INODE_PER_BLOCK + 1;
    uint64_t offset = (inum - 1) % INODE_PER_BLOCK;
    read_disk(block_id, data);
    inode *inodes = (inode *)data;
    if(node->flag!=1){
        printf("inode %d is not allocated\n",inum);
    }
    printf("write inode %d %d\n",inum,in_mem_inode[inum-1].flag);
    assert(node->flag==1);
    /* length is used to escape zero out the memory when allocate inode in user space */
    cp_inode(&inodes[offset], node);
    cp_inode(in_mem_inode + ( inum - 1 ),node);
    write_disk(block_id, data);
}

void init_free_disk(int start)
{
	char data[BLOCKSIZE];
    char test[BLOCKSIZE];
	int block_entries_per_block = BLOCKSIZE / BLOCKADDR; // num of addr of block per block
	int record_block_count = (BLOCKCOUNT - i_list_block_count - 1) / block_entries_per_block;
	uint64_t *datablock = (uint64_t *)data;
	int block_id = start;

	head = start;

	while (record_block_count--)
	{
		block_id = start;
		for (int i = 1; i < block_entries_per_block; i++)
		{
			start++;
			datablock[i] = start;
		}
		start++;
		datablock[0] = start;
		write_disk(block_id, datablock);
        read_disk(block_id,test);
        uint64_t * test_block = (uint64_t*) test;
        //printf("block id %d:\n",block_id);
        /*for(int j = 0;j<(BLOCKSIZE/BLOCKADDR);j++){
            printf("%ld ",test_block[j]);
        }
        printf("\n");*/
	}

	if ((BLOCKCOUNT - i_list_block_count - 1) % block_entries_per_block != 0)
	{
		int block_id = start;
		memset(data, 0, BLOCKSIZE);
		for (int i = 1; i < block_entries_per_block; i++)
		{
			start++;
			if (start > (BLOCKCOUNT - 1))
				datablock[i] = 0;
			else
				datablock[i] = start;
		}
		datablock[0] = 0;
		write_disk(block_id, datablock);
        read_disk(block_id,test);
        uint64_t * test_block = (uint64_t*) test;
        /*printf("block id %d:\n",block_id);
        for(int j = 0;j<(BLOCKSIZE/BLOCKADDR);j++){
            printf("%ld ",test_block[j]);
        }
        printf("\n");*/
	}
	else
	{
		datablock[0] = 0;
		write_disk(block_id, datablock); //rewrite the last block
        read_disk(block_id,test);
        uint64_t * test_block = (uint64_t*) test;
        /*printf("block id %d:\n",block_id);
        for(int j = 0;j<(BLOCKSIZE/BLOCKADDR);j++){
            printf("%ld ",test_block[j]);
        }
        printf("\n");*/
	}
}

void init_i_list()
{
	char tmp[BLOCKSIZE];
	memset(tmp, 0, sizeof(tmp));
	i_list_block_count = BLOCKCOUNT / 10;
    in_mem_inode = (inode*)malloc(i_list_block_count*INODE_PER_BLOCK*sizeof(inode));
	for (int i = 1; i <= i_list_block_count; i++)
	{
		write_disk(i, tmp);
	}
    memset(in_mem_inode,0,i_list_block_count*INODE_PER_BLOCK*sizeof(inode));
}

void mkfs()
{
	char tmp[BLOCKSIZE];

	memset(tmp, 0, sizeof(tmp));

	init_i_list();

	uint64_t *supernode = (uint64_t *)tmp;

	//int start = INODES / INODE_PER_BLOCK + 1;
	int start = i_list_block_count + 1;
	//start of block
	supernode[0] = start;
	supernode[1] = i_list_block_count; //inode block count
	supernode[2] = BLOCKSIZE;
	supernode[3] = 1; //init flag
	write_disk(0, tmp);

	printf("data block starts from block %d\n", start);
	init_free_disk(start);

}

/*
 * read from disk
 * not needed any more
 * */
void read_inode_old(uint64_t inum, inode *node)
{
	char data[BLOCKSIZE];
	uint32_t block_id = 1 + (inum - 1) / INODE_PER_BLOCK;
	uint32_t offset = (inum - 1) % INODE_PER_BLOCK;
    assert(block_id<=i_list_block_count);
	read_disk(block_id, data);
	inode *inodes = (inode*) data;
	cp_inode(node, &inodes[offset]);
}
/*
 * read from memory, the inode id starts from 1
 * */
void read_inode(uint64_t inum,inode *node){
    uint32_t block_id = 1 + (inum - 1) / INODE_PER_BLOCK;
    assert(block_id<=i_list_block_count);
    cp_inode(node,in_mem_inode + (inum - 1));
}

/*
 * zero for failure
 * */
uint64_t allocate_inode()
{
	char tmp[BLOCKSIZE];
	int i, j, flag = 0;
	inode *inode_block = (inode*) tmp;
	for (i = 1; i <= i_list_block_count*INODE_PER_BLOCK; i++)
	{
		if(in_mem_inode[i-1].flag==0) {
            in_mem_inode[i-1].flag = 1;
            write_inode(i,in_mem_inode+(i-1));
            flag = 1;
            break;
        }
	}
	if(!flag) {
        printf("inode full\n");
        i = 0;
    }
	return i;
}

int free_inode(uint64_t inum)
{
	uint64_t block_id = 1 + (inum - 1) / INODE_PER_BLOCK;
	uint32_t offset = (inum - 1) % INODE_PER_BLOCK;
	inode tmp;
	if (block_id > i_list_block_count)
	{
		printf("free block_id too big %ld \n", inum);
		return -1;
	}
	read_block(block_id, sizeof(inode) * offset, sizeof(inode), &tmp);

	for (int i = 0; i < DIRECT_BLOCK; i++)
	{
		if(tmp.direct_blocks[i]) {
			free_data_block(tmp.direct_blocks[i]);
			tmp.direct_blocks[i] = 0;
		}
		else
			break;
	}
	for (int i = 0; i < SING_INDIR; i++)
	{
		if(tmp.sing_indirect_blocks[i]) {
			free_data_block(tmp.sing_indirect_blocks[i] = 0);
			tmp.sing_indirect_blocks[i] = 0;
		}
		else
			break;
	}
	for (int i = 0; i < DOUB_INDIR; i++){
		if(tmp.doub_indirect_blocks[i]) {
			free_data_block(tmp.doub_indirect_blocks[i] = 0);
			tmp.doub_indirect_blocks[i] = 0;
		}
		else
			break;
	}
	for (int i = 0; i < TRIP_INDIR; i++){
		if(tmp.trip_indirect_blocks[i]) {
			free_data_block(tmp.trip_indirect_blocks[i] = 0);
			tmp.trip_indirect_blocks[i] = 0;
		}
		else
			break;
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
	for (i = 1; i < (BLOCKSIZE / BLOCKADDR); i++)
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
		write_disk(head, tmp);
	}
	else
	{
		res = head;
		head = data[0];
		data[0] = 0;
		write_block(0, 0, sizeof(uint64_t), &head);
		write_disk(res, tmp);
	}
	return res;
}

int free_data_block(uint64_t id)
{
	uint8_t tmp[BLOCKSIZE];
	uint64_t *data = (uint64_t *)tmp;
	read_block(head, 0, BLOCKSIZE, tmp);
	int i = 0, flag = 0;
	for (i = 1; i < (BLOCKSIZE / BLOCKADDR); i++)
	{
		if (data[i] == 0)
		{
			flag = 1;
			data[i] = id;
			break;
		}
	}

	if (flag == 1)
		write_disk(head, tmp);
	else
	{
		uint64_t pre_head = head;
        printf("pre head %ld\n",pre_head);
		head = id;
		write_block(pre_head, 0, sizeof(uint64_t), &id);
	}
	return 0;
}

/* when to write back is a serious question */
int read_block(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
	char tmp[BLOCKSIZE];

	if (offset > BLOCKSIZE)
	{
		printf("offset %ld in write_disk greate than BLOCKSIZE\n", offset);
		return -1;
	}

	read_disk(block_id, tmp);
	memcpy((char *)buffer, tmp + offset, fmin(size, BLOCKSIZE - offset));

	return fmin(size, BLOCKSIZE - offset);
}

int write_block(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
	char tmp[BLOCKSIZE];

	if (offset > BLOCKSIZE)
	{
		printf("boundary exceeded %ld in write_disk greate than BLOCKSIZE\n", offset);
		return -1;
	}

	read_disk(block_id, tmp);
	memcpy(tmp + offset, (char *)buffer, fmin(size, BLOCKSIZE - offset));

	write_disk(block_id, tmp);

	return fmin(size, BLOCKSIZE - offset);
}


/* size is for computation convinent */