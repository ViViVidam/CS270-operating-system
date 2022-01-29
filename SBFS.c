#include "nodes.h"
#include "SBFS.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MAX_OPEN 100
#define H_CREATE 1
#define H_READ 2

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

#define DIR_LENG (BLOCKSIZE / sizeof(dir))
uint64_t block_id_helper(inode *node, int index, int mode);

uint64_t find_file_entry(uint64_t inum, char *filename)
{
    inode node;
    int j = 0;
    read_inode(inum,&node);
    int entry_count = node.size / sizeof (dir);
    assert( (node.size%sizeof(dir)) == 0 );
    for(int i = 0; i < entry_count ; i++ )
    {
        dir entry;
        int index = i * sizeof(dir);
        int offset = i * sizeof(dir) % BLOCKSIZE;
        read_block(block_id_helper(&node,index,H_READ),offset,sizeof(dir),&entry);
        if(strcmp(entry.filename,filename)==0){
            return entry.inum;
        }
    }
    return 0;
}

uint64_t SBFS_namei(char *path)
{
	char buf[BLOCKSIZE];
	char filename[MAX_FILENAME];
	char *pointer = path;
	int i = 0;
	uint64_t block_id;

	pointer += 1;
	block_id = ROOT;

	while (pointer != 0)
	{
		if (*pointer != '/')
		{
			filename[i++] = *pointer;
			pointer += 1;
		}
		filename[i] = 0;
		block_id = find_file_entry(block_id, filename);
		if (block_id == 0)
			return 0; //cannot find inode
		if (*pointer == '/') {
            i = 0;
            pointer++;
        }
		else
			return block_id;
	}

	return 1; // the path end with /
}

int SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	char *buffer = (char *)buf;
	inode node;
	read_inode(inum, &node);

	int start = offset / BLOCKSIZE;
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int read_bytes = read_block(start, block_offset, size, buf);
	size -= read_bytes;
	buffer += read_bytes;

	while (size > 0)
	{
		start += 1;
		int block_id = block_id_helper(&node, start, H_CREATE);
		assert(block_id != 0);
		read_bytes = read_block(block_id, block_offset, size, buf);
		buffer += read_bytes;
		size -= read_bytes;
	}
	return 0;
}
/* mode H_READ, don't create new block,mode H_CREATE create new block */

int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	inode node;
	read_inode(inum, &node);
	char *buffer = (char *)buf;

	int start = offset / BLOCKSIZE;
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int write_bytes = write_block(start, block_offset, size, buf);
	size -= write_bytes;
	buffer += write_bytes;

	while (size > 0)
	{
		start += 1;
        int block_id = block_id_helper(&node, start, H_CREATE);
		write_bytes = write_block(block_id, block_offset, size, buf);
		buffer += write_bytes;
		size -= write_bytes;
	}
	node.size = MAX(node.size, offset + size);
	write_inode(inum, &node);
	return 0;
}

/* direcotry is dir, the item is empty when inode = 0 */
uint64_t SBFS_mkdir(char *filename, inode *node)
{

	/* writing into new dir */
	uint64_t inum = allocate_inode();
	memset(node, 0, sizeof(inode));
	node->type = DIRECTORY;
	node->size = 0;
	node->flag = 1;
	return inum;
}

uint64_t SBFS_mknod(char *filename, inode *node)
{
	uint64_t inum = allocate_inode();
	memset(node, 0, sizeof(inode));
	node->type = NORMAL;
	node->size = 0;
	node->flag = 1;
	return inum;
}

int SBFS_unlink(char *path)
{
	inode node;
	uint64_t inum = SBFS_namei(path);
	read_inode(inum, &node);
	int item_count = node.size / sizeof(dir);
	assert((node.size % sizeof(dir)) == 0);

	dir entry;
	int offset = 0;
	if (node.type == DIRECTORY)
	{
		for (int i = 0; i < item_count; i++)
		{
			int index = i * sizeof(dir) / BLOCKSIZE;
			int offset = i * sizeof(dir) % BLOCKSIZE;
            int block_id = block_id_helper(index,&node,H_READ);
            assert(block_id!=0);
			SBFS_read(block_id, offset, sizeof(dir), &entry);
			assert(entry.inum != 0);
			free_inode(entry.inum);
		}
	}
	free_inode(inum);
	return 0;
}

int SBFS_close(int node)
{
	return 0;
}

uint64_t SBFS_open(char *filename, int mode)
{
	uint64_t inode = SBFS_namei(filename);
	return inode;
}

void SBFS_init()
{
	mkfs();
}

dir *SBFS_readdir(uint64_t inum)
{
	static int i;
	static int present_inum;
	static int item_count;
	static inode node;
	if (inum != present_inum)
	{
		i = 0;
		present_inum = inum;
		read_inode(inum, &node);
		assert(node.type == DIRECTORY);
	}
	item_count = node.size / sizeof(dir);
	assert((node.size % sizeof(dir)) == 0);

	dir entry;
	if (i >= item_count)
		return NULL;
	int index = i * sizeof(dir) / BLOCKSIZE;
	int offset = i * sizeof(dir) % BLOCKSIZE;
    int block_id = block_id_helper(index,&node,H_READ);
	SBFS_read(block_id, offset, sizeof(dir), &entry);
	i += 1;
	assert(entry.inum != 0);
    return &entry;
}

uint64_t block_id_helper(inode *node, int index, int mode)
{
	char tmp[BLOCKSIZE];
	char zero[BLOCKSIZE];
	memset(zero, 0, BLOCKSIZE);
	uint64_t *address = (uint64_t *)tmp;
	int flag = 0; // indicate is there a change in inode
	if (index < DIRECT_BLOCK)
	{
		if (node->direct_blocks[index] == 0 && mode == H_CREATE)
		{
			node->direct_blocks[index] = allocate_data_block();
		}
		return node->direct_blocks[index];
	}
	else if (index < (DIRECT_BLOCK + SING_INDIR * 512))
	{
		if (node->sing_indirect_blocks[0] == 0 && mode == H_CREATE)
		{
			if (mode == H_CREATE)
			{
				node->sing_indirect_blocks[0] = allocate_data_block();
				write_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, zero);
			}
			else
				return 0;
		}
		// I/O can be cut down, but I will leave it for now
		read_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, tmp);
		if (address[index - DIRECT_BLOCK] == 0 && mode == H_CREATE)
		{
			address[index - DIRECT_BLOCK] = allocate_data_block();
			write_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, tmp);
		}
		return address[index - DIRECT_BLOCK];
	}
	else if (index < (DIRECT_BLOCK + SING_INDIR * 512 + DOUB_INDIR * 512 * 512))
	{
		if (node->doub_indirect_blocks[0] == 0)
		{
			if (mode == H_CREATE)
			{
				node->doub_indirect_blocks[0] = allocate_data_block();
				write_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, zero);
			}
			else
				return 0;
		}
		read_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / 512;
		if (address[next_level_index] == 0)
		{
			if (mode == H_CREATE)
			{
				address[next_level_index] = allocate_data_block();
				write_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, tmp);
				write_block(address[next_level_index], 0, BLOCKSIZE, zero);
			}
			else
				return 0;
		}
		int parent_id = address[next_level_index];
		read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR * 512) % 512;
		if (address[index] == 0 && mode == H_CREATE)
		{
			address[index] = allocate_data_block();
			write_block(parent_id, 0, BLOCKSIZE, tmp);
		}
		return address[index];
	}
	else
	{
		if (node->trip_indirect_blocks[0] == 0)
		{
			if (mode == H_CREATE)
			{
				node->trip_indirect_blocks[0] = allocate_data_block();
				write_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, zero);
			}
			else
				return 0;
		}
		read_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / (512 * 512);
		if (address[next_level_index] == 0)
		{
			if (mode == H_CREATE)
			{
				address[next_level_index] = allocate_data_block();
				write_block(address[next_level_index], 0, BLOCKSIZE, zero);
				write_block(node->trip_indirect_blocks[0],0,BLOCKSIZE, tmp);
			}
			else
				return 0;
		}
		int parent_id = address[next_level_index];
		read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
		next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / 512;
		if (address[next_level_index] == 0)
		{
			if (mode == H_CREATE)
			{
				address[next_level_index] = allocate_data_block();
				write_block(address[next_level_index], 0, BLOCKSIZE, zero);
				write_block(parent_id, 0, BLOCKSIZE, tmp);
			}
			else
				return 0;
		}
		parent_id = address[next_level_index];
		read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR * 512) % 512;
		if (address[index] == 0 && mode == H_CREATE)
		{
			address[index] = allocate_data_block();
			write_block(parent_id, 0, BLOCKSIZE, tmp);
		}
		return address[index];
	}
}

int main()
{
    char test[BLOCKSIZE];
    mkfs();
    inode node;
    for(int i = 0; i < 256; i++){
        uint64_t tmp1 = allocate_inode();
        read_inode(tmp1,&node);
        //printf("%d\n",tmp1);
        node.size = 56;
        node.direct_blocks[0] = 678;
        write_inode(tmp1,&node);
        //printf("2\n");
        read_inode(tmp1,&node);
        printf("id:%ld (%ld %ld %ld)\n",tmp1,node.size,node.direct_blocks[0],node.flag);
    }
    for(int i = 0; i < 259; i++){
        uint64_t tmp1 = free_inode(i);
        read_inode(i,&node);
        printf("id:%ld (%ld %ld %ld)\n",i,node.size,node.direct_blocks[0],node.flag);
    }
    return 0;
}