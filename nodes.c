#include <stdio.h>
#include <string.h>
#include "nodes.h"
#include <math.h>
#include <assert.h>

#define INODE_PER_BLOCK (BLOCKSIZE / (sizeof(inode) / 8))

void cp_inode(inode *dest, inode *source)
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
	cp_inode(&inodes[offset], node);
	write_disk(block_id, data);
}

void init_free_disk(int start)
{
	char data[BLOCKSIZE];
	int block_entries_per_block = BLOCKSIZE / BLOCKADDR; // num of addr of block per block
	int record_block_count = (BLOCKCOUNT - i_list_block_count - 1) / block_entries_per_block;
	uint64_t *datablock = (uint64_t *)data;
	int block_id = start;

	head = start;

	while (record_block_count--)
	{
		block_id = start;
		//memset(data, 0, BLOCKSIZE);
		for (int i = 1; i < block_entries_per_block; i++)
		{
			start++;
			datablock[i] = start + 1;
		}
		start++;
		datablock[0] = start;
		write_disk(block_id, datablock);
	}

	if ((BLOCKCOUNT - i_list_block_count - 1) % block_entries_per_block != 0)
	{
		int block_id = start;
		memset(data, 0, BLOCKSIZE);
		for (int i = 1; i < block_entries_per_block; i++)
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
		write_disk(i, tmp);
	}
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
	read_disk(block_id, data);
	inode *inodes = (inode*) data;
	cp_inode(node, &inodes[offset]);
}

/* zero for failure */
uint64_t allocate_inode()
{
	char tmp[BLOCKSIZE];
	int i, j, flag, block_id;
	inode *inode_block = (inode*) tmp;
	for (i = 1; i <= i_list_block_count; i++)
	{
		read_disk(i, tmp);
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
		write_disk(i,tmp);
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
		printf("block_id too big %ld \n", inum);
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
		write_disk(head, tmp);
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