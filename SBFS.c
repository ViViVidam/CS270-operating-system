#include "nodes.h"
#include "SBSF.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_OPEN 100
#define DIR 0
#define FILE 1
#define MIN(a,b) ((a>b)?b:a)
typedef struct{
	char filename[MAX_FILENAME];
	uint64_t inum;
} dir;


#define DIR_LENG (BLOCKSIZE/sizeof(dir))

file_describ open_file_table[MAX_OPEN];
inode_describ ilist[INODES];

int get_inode_node(inode* node,uint64_t index,void* buf){
	if(index<DIRECT_BLOCK){
		buf = get_inode()
	}
}

uint64_t find_file_entry(uint64_t block_id, char* filename){
	int j = 0;
	while(read_block(block_id,j++,buf)){
		dir* files = (dir*) buf;
		for(int k = 0; k < DIR_LENG; k++){
			if(strcmp(files[k].filename,filename)==0)
				return files[k].inode;
			else if(files[k].inode==0){
				return 0;//fail to find
			}
		}
	}
}

uint64_t SBFS_namei(char* path){
	char buf[BLOCKSIZE];
	char filename[MAX_FILENAME];
	char* pointer = path;
	int i = 0;
	uint64_t block_id;

	pointer += 1;
	block_id = ROOT;

	while( pointer!= 0){ 
		if(*pointer != '/'){
			filename[i++] = *pointer;
			pointer += 1; 
		}
		filename[i] = 0;
		block_id = find_file_entry(block_id,filename);
		if(block_id == 0)
			return 0;//cannot find inode
		if(pointer=='/')
			i = 0;
		else
			return block_id;
	}

	return 1;// the path end with /
}

int SBFS_read(uint64_t block_id,uint64_t offset,int64_t size,void* buf){
	//uint64_t block_id = SBFS_namei(filename);
	char* buffer = (char*) buf;
	char data[BLOCKSIZE];

	assert(block_id>i_list_size);
	inode* tmp = get_inode(block_id);
	int start = offset / BLOCKSIZE;
	uint64_t block_offset = offset % BLOCKSIZE;	
	
	assert(offset<BLOCKSIZE);
	while(size>0){
		if(read_block(tmp,start,data)){
			int cp_size = MIN(size,BLOCKSIZE-offset);
			memcpy(buffer,data+offset,cp_size);
			buffer += cp_size;
			size -= cp_size;
			start += 1;
		}
		else{
			printf("SBFS_read encountered zero in read_block %d\n",start);
			return -1;
		}
	}
	return 0;
}

int SBFS_write(int inum,uint64_t offset,int64_t size,void* buf){
	char* buffer = (char*) buf;
	char data[BLOCKSIZE];

	assert(inum>i_list_size);
	int start = offset / BLOCKSIZE;
	uint64_t block_offset = offset % BLOCKSIZE;	
	
	assert(offset<BLOCKSIZE);
	while(size>0){
		if(read_block(inum,start,data)){
			int cp_size = MIN(size,BLOCKSIZE-offset);
			memcpy(data+offset,buffer,cp_size);
			buffer += cp_size;
			size -= cp_size;
			write_block(block_id,start,data,size);
			start += 1;
		}
		else{
			printf("SBFS_read encountered zero in read_block %d\n",start);
			return -1;
		}
	}
	return 0;
}

/* direcotry is dir, but in each block has to be ended in inode 0 */
int SBFS_mkdir(char* path, char* filename){

	/* writing into new dir */
	uint64_t inum = allocate_inode();
	inode node;//no need to set node.flag = 1;
	uint64_t datablock = allocate_data_block();
	inode node.direct_blocks[0] = datablock;
	write_inode(inum,&node,1);
	char data[BLOCKSIZE];

	dir new_dir;
	new_dir.inum = 0;	
	SBFS_write(datablock,0,sizeof(dir),&new_dir);

	int parent_inum = SBFS_namei(path);
	read_inode(parent_inum,&node);
	SBFS_read(parent_inum,(node->size-sizeof(dir)),&new_dir);
	strcpy(new_dir.filename,filename);
	new_dir.inum = inum;
	SBFS_write(parent_inum,(node->size-sizeof(dir),&new_dir));
	strcpy(new_dir.filename,"");
	new_dir.inum = 0;
	SBFS_write(parent_inum,node->size,&new_dir);

	return inum;
}



uint64_t SBFS_open(char* filename,int mode){
	uint64_t inode = SBFS_namei(filename);
	return inode;
}

int SBFS_init(){
	mkfs();
}