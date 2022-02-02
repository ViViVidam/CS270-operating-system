#include "SBFS.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define H_CREATE 1
#define H_READ 2

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

#define DIR_LENG (BLOCKSIZE / sizeof(dir))
uint64_t block_id_helper(inode *node, int index, int mode);
void SBFS_readdir_raw(uint64_t inum, int entry_index, dir *entry);
void create_root_dir();

char *SBFS_clean_path(char *path)
{
	int state = 0;
	char tmp[4096];
	int i = 0;
	while (*path != 0)
	{
		if (state == 0)
		{
			if (*path == '/')
			{
				state = 1;
			}
			tmp[i++] = *path;
		}
		else
		{
			if (*path != '/')
			{
				tmp[i++] = *path;
				state = 0;
			}
		}
		path += 1;
	}
	tmp[i] = 0;
}
void SBFS_readdir_raw(uint64_t inum, int entry_index, dir *entry)
{
	SBFS_read(inum, entry_index * sizeof(dir), sizeof(dir), entry);
}

uint64_t add_entry_to_dir(uint64_t dir_inum, char *filename, uint64_t file_inum)
{
	inode node;
	dir entry;
	read_inode(dir_inum, &node);
	int entries = node.size / sizeof(dir);
	assert(node.size % sizeof(dir) == 0);
	int i = 0;
	for (i = 0; i < entries; i++)
	{
		SBFS_readdir_raw(dir_inum, i, &entry);
		if (entry.inum == 0)
		{
			strcpy(entry.filename, filename);
			entry.inum = file_inum;
			SBFS_write(dir_inum, i * sizeof(entry), sizeof(entry), &entry);
			return i;
		}
	}
	strcpy(entry.filename, filename);
	entry.inum = file_inum;
	SBFS_write(dir_inum, i * sizeof(entry), sizeof(entry), &entry);
	return i;
}

int delete_entry_from_dir(uint64_t dir_inum, uint64_t file_inum)
{
    inode node;
    dir entry;
    read_inode(dir_inum, &node);
    int entries = node.size / sizeof(dir);
    assert(node.size % sizeof(dir) == 0);
    int i = 0;
    for (i = 0; i < entries; i++)
    {
        SBFS_readdir_raw(dir_inum, i, &entry);
        printf("entry num,%ld\n",entry.inum);
        if (entry.inum == file_inum)
        {
            printf("%ld\n",i);
            entry.inum = 0;
            SBFS_write(dir_inum, i * sizeof(entry), sizeof(entry), &entry);
            return i;
        }
    }
    return -1;
}

uint64_t find_file_entry(uint64_t inum, char *filename)
{
	inode node;
	read_inode(inum, &node);
	assert(node.type == DIRECTORY);
	int entry_count = node.size / sizeof(dir);
	dir entry;
	assert((node.size % sizeof(dir)) == 0);
	for (int i = 0; i < entry_count; i++)
	{
		SBFS_readdir_raw(inum, i, &entry);
		if (strcmp(filename, entry.filename) == 0)
			return entry.inum;
	}
	return 0;
}

uint64_t SBFS_namei(char *path)
{
	char buf[BLOCKSIZE];
	char filename[MAX_FILENAME];
	char *pointer = path;
	int i = 0;
	uint64_t inum;

	if (strcmp(path, "/") == 0)
	{
		return ROOT;
	}
	pointer += 1;
	inum = ROOT;

	while (*pointer != 0)
	{
		while (*pointer != '/' && *pointer != 0)
		{
			filename[i++] = *pointer;
			pointer += 1;
		}
		filename[i] = 0;
		inum = find_file_entry(inum, filename);
		if (inum == 0)
		{
			printf("namei, failed to find path: %s %s\n", path, filename);
			return 0; //cannot find inode
		}
		//changed!!
		if (*pointer == '/')
		{
			inode node;
			read_inode(inum, &node);
			if (node.type != DIRECTORY)
			{
				printf("%sit is not a dir\n", filename);
				return 0;
			}
		}
		while (*pointer == '/')
		{
			i = 0;
			pointer++;
		}
		if (*pointer == 0)
			return inum;
	}
	return 1; // the path end with /
}

//TODO: 是不是要return read size
int SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	char *buffer = (char *)buf;
	inode node;
	read_inode(inum, &node);
	int64_t pre_size = size;

	int start = block_id_helper(&node, offset / BLOCKSIZE, H_READ);
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
	return pre_size;
}
/* mode H_READ, don't create new block,mode H_CREATE create new block
 * the corresponding inode where and what size
 * */

int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	inode node;
	read_inode(inum, &node);

	char *buffer = (char *)buf;
	uint64_t upperbound = size + offset;
	int start = block_id_helper(&node, offset / BLOCKSIZE, H_CREATE);
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

	node.size = MAX(node.size, upperbound);
	write_inode(inum, &node);
	read_inode(inum, &node);
	return size;
}

int find_slash(char *path, int pos)
{
	char *pointer = path + pos;
	int res = pos;
	while (*pointer != 0 && *pointer != '/')
	{
		++res;
		pointer++;
	}
	if (*pointer == 0)
	{
		return -1;
	}
	return res;
}


uint64_t find_parent_dir_inum(char *path)
{
    char parent_path[MAX_PATH];
    int len = strlen(path);
    int i = 0;
    while (*(path + len - 1) != '/'){
        len--;
    }
    memcpy(parent_path,path,len);
    if(len==1)
        parent_path[len] = 0;
    else
        parent_path[len-1] = 0;
    char* filename = path + len;
    printf("path %s filename %s\n",parent_path,filename);
    uint64_t parent_path_inum = SBFS_namei(parent_path);
    printf("%ld\n",parent_path_inum);
    return parent_path_inum;
}
/* direcotry is dir, the item is empty when inode = 0 */
/*
return 0: can not mkdir
*/
uint64_t SBFS_mkdir(char *path)
{
	char parent_path[5 * MAX_FILENAME];
	if (SBFS_namei(path) != 0)
	{
		return 0;
	}
	int len = strlen(path);
	int i = 0;
	while (*(path + len - 1) != '/')
	{
		len--;
	}
    memcpy(parent_path,path,len);
    if(len==1)
        parent_path[len] = 0;
    else
        parent_path[len-1] = 0;
    char* filename = path + len;
    printf("path %s filename %s\n",parent_path,filename);
    uint64_t parent_path_inum = SBFS_namei(parent_path);
    uint64_t child_inum;
    if (parent_path_inum == 0)
    {
        return 0;
    }
    else
    {
        inode parent_node;
        read_inode(parent_path_inum, &parent_node);
        if (parent_node.type != DIRECTORY)
        {
            printf("path:%s not a dir\n",parent_path);
            return 0;
        }
        else
        {
            inode node;
            child_inum = allocate_inode();
            read_inode(child_inum,&node);
            node.type = DIRECTORY;
            node.size = 0;
            write_inode(child_inum, &node);
            add_entry_to_dir(parent_path_inum, filename, child_inum);
        }
    }
	return child_inum;
}

uint64_t SBFS_mknod(char *path)
{
    char parent_path[5*MAX_FILENAME];
    if (SBFS_namei(path) != 0)
    {
        return 0;
    }
    int len = strlen(path);
    int i = 0;
    while (*(path + len - 1) != '/'){
        len--;
    }
    memcpy(parent_path,path,len);
    if(len==1)
        parent_path[len] = 0;
    else
        parent_path[len-1] = 0;
    char* filename = path + len;
    printf("path %s filename %s\n",parent_path,filename);
    uint64_t parent_path_inum = SBFS_namei(parent_path);
    uint64_t child_inum;
    if (parent_path_inum == 0)
    {
        return 0;
    }
    else
    {
		inode parent_node;
		read_inode(parent_path_inum, &parent_node);
		if (parent_node.type != DIRECTORY)
		{
			return 0;
		}
		else
		{
			inode node;
			child_inum = allocate_inode();
			read_inode(child_inum, &node);
			node.type = NORMAL;
			node.size = 0;
			write_inode(child_inum, &node);
			add_entry_to_dir(parent_path_inum, filename, child_inum);
		}
	}
	return child_inum;
}
// rmdir - remove empty directories

//TODO: delete from parent dir

/******
**  rmdir - remove empty directories
******/
int SBFS_rmdir(char *path)
{
	inode node;
	uint64_t inum = SBFS_namei(path);

	if (inum == 0)
	{
		printf("\nlocate failed for path: %s\n", path);
		return -1;
	}
	printf("rmdir %s: inum %ld\n", path, inum);

	read_inode(inum, &node);
	if (node.type != DIRECTORY || node.size != 0)
	{
		printf("\npath: %s is not an empty directory.\n", path);
		return -1;
	}

	uint64_t parent_dir_inum = find_parent_dir_inum(path);
    printf("rmdir %ld: inum %ld\n", parent_dir_inum, inum);
    delete_entry_from_dir(parent_dir_inum, inum);
	free_inode(inum);
	return 0;
}

/******
** unlink - call the unlink function to remove the specified file
******/
int SBFS_unlink(char *path)
{
	inode node;
	uint64_t inum = SBFS_namei(path);

	if (inum == 0)
	{
		printf("\nlocate failed for path: %s\n", path);
		return -1;
	}
	printf("unlink %ld\n", inum);
	read_inode(inum, &node);
	uint64_t parent_path_inum ;

	if (node.type != NORMAL)
	{
		printf("\n%s is not a file\n", path);
		return -1;
	}
	else
	{
		parent_path_inum = find_parent_dir_inum(path);
        printf("unlink parent inum %ld\n",parent_path_inum);
		delete_entry_from_dir(parent_path_inum, inum);
		free_inode(inum);
	}
	return 0;
}

int SBFS_close(int inum)
{
	return 0;
}

uint64_t SBFS_open(char *filename, int mode)
{
	uint64_t inum = SBFS_namei(filename);
	return inum;
}

void SBFS_init()
{
	mkfs();
	create_root_dir();
}

dir *SBFS_readdir(uint64_t inum,int init)
{

	static int i;
	static int present_inum = -1;
	static int item_count;
	static inode node;
	static dir entry;
    if(init != 1) {
        if (inum != present_inum) {
            i = 0;
            present_inum = inum;
            read_inode(inum, &node);
            assert(node.type == DIRECTORY);
            item_count = node.size / sizeof(dir);
        }
        printf("SBFS_readdir inum %ld itemcount %ld %ld\n", inum, node.size, item_count);
        assert((node.size % sizeof(dir)) == 0);
        while (1) {
            if (i >= item_count)
                return NULL;
            SBFS_readdir_raw(inum, i, &entry);
            printf("diur %ld %s %ld\n", inum, entry.filename, entry.inum);
            i += 1;
            if (entry.inum != 0) {
                return &entry;
            }
        }
        return NULL;
    }
    else{
        i = 0;
        present_inum = -1;
        item_count = 0;
        memset(&node,0,sizeof(node));
        memset(&entry,0,sizeof(entry));
    }
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
				write_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, tmp);
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

void create_root_dir()
{
	inode root_node;
	uint64_t root = allocate_inode();
	assert(root == ROOT);
	read_inode(root, &root_node);
	root_node.size = 0;
	root_node.type = DIRECTORY;
	write_inode(root, &root_node);
}
/*int main()
{
	dir *entry;
	SBFS_init();
    uint64_t inum = SBFS_mknod("/abc");
	//add_entry_to_dir(ROOT, "testtest1", inum);
	//inum = SBFS_mkdir("/testtest2", &node);
	//add_entry_to_dir(ROOT, "testtest2", inum);
	//inum = SBFS_mknod("/testtest3", &node);
	//add_entry_to_dir(ROOT, "testtest3", inum);
	SBFS_rmdir("/testtest2");
    SBFS_unlink("/test123");
	while (entry = SBFS_readdir(ROOT))
	{
		printf("readdir %s %ld\n", entry->filename, entry->inum);
	}
	return 0;
}*/