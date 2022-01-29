#include "nodes.h"
#include "SBSF.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_OPEN 100
#define DIR 0
#define FILE 1
#define H_CREATE 1
#define H_READ 2

#define MIN(a,b) ((a>b)?b:a)
typedef struct{
	char filename[MAX_FILENAME];
	uint64_t inum;
	uint8_t valid;
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
/* mode H_READ, don't create new block,mode H_CREATE create new block */
uint64_t block_id_helper(inode* node,int index,int mode){
	char tmp[BLOCKSIZE];
	char zero[BLOCKSIZE];
	memset(zero,0,BLOCKSIZE);
	uint64_t* address = (uint64_t*)tmp;
	int flag = 0;// indicate is there a change in inode
	if(index<DIRECT_BLOCK){
		if(node->direct_blocks[index]==0 && mode==H_CREATE){
			node->direct_blocks[index] = allocate_data_block();
		}
		return node->direct_blocks[index];
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512) ){
		if(node->sing_indirect_blocks[0]==0 && mode==H_CREATE){
			if(mode == H_CREATE){
				node->sing_indirect_blocks[0] = allocate_data_block();
				write_block(node->sing_indirect_blocks[0],0,BLOCKSIZE,zero);
			}
			else
				return 0;
		}
		// I/O can be cut down, but I will leave it for now
		read_disk(node->sing_indirect_blocks[0],0,BLOCKSIZE,tmp);
		if(address[index-DIRECT_BLOCK]==0 && mode==H_CREATE){
			address = allocate_data_block();
			write_block(node->sing_indirect_blocks[0],0,BLOCKSIZE,tmp);
		}
		return address[index-DIRECT_BLOCK];
	}
	else if(index< (DIRECT_BLOCK+SING_INDIR*512+DOUB_INDIR*512*512)){
		if(node->doub_indirect_blocks[0]==0){
			if(mode == H_CREATE){
				node->doub_indirect_blocks[0] = allocate_data_block();
				write_block(node->doub_indirect_blocks[0],0,BLOCKSIZE,zero);
			}
			else
				return 0;
		}
		read_disk(node->doub_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/512;
		if(address[next_level_index]==0){
			if(mode == H_CREATE){
				address[next_level_index] = allocate_data_block();
				write_block(node->doub_indirect_blocks[0],0,BLOCKSIZE,tmp);
				write_block(address[next_level_index],0,BLOCKSIZE,zero);
			}
			else
				return 0;
		}
		int parent_id = address[next_level_index];
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		index = (index - DIRECT_BLOCK - SING_INDIR*512)%512;
		if(address[index]==0 && mode==H_CREATE){
			address[index] = allocate_data_block();
			write_block(parent_id,0,BLOCKSIZE,tmp);
		}
		return address[index];
	}
	else{
		if(node->trip_indirect_blocks[0]==0){
			if(mode == H_CREATE){
				node->trip_indirect_blocks[0] = allocate_data_block();
				write_block(node.trip_indirect_blocks[0],0,BLOCKSIZE,zero);
			}
			else
				return 0;
		}
		read_disk(node->trip_indirect_blocks[0],0,BLOCKSIZE,tmp);
		int next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ (512*512);
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_block(address[next_level_index],0,BLOCKSIZE,zero);
			write_block(node->trip_indirect_blocks[0].0.BLOCKSIZE,tmp);
		}
		int parent_id = address[next_level_index];
		read_disk(address[next_level_index],0,BLOCKSIZE,tmp);
		next_level_index = (index - DIRECT_BLOCK - SING_INDIR*512)/ 512;
		if(address[next_level_index]==0){
			address[next_level_index] = allocate_data_block();
			write_block(address[next_level_index],0,BLOCKSIZE,zero);
			write_block(parent_id,0,BLOCKSIZE,tmp);
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
}

int SBFS_write(int inum,uint64_t offset,int64_t size,void* buf){
	char* buffer = (char*) buf;
	inode node;
	read_inode(inum,&node);

	assert(inum>i_list_size);
	int start = offset / BLOCKSIZE;
	uint64_t block_offset = offset % BLOCKSIZE;	
	assert(block_offset<BLOCKSIZE);

	while(size>0){
		block_id_helper(&node,start);
		int cp_size = MIN(size,BLOCKSIZE-offset);
		memcpy(data+offset,buffer,cp_size);
		buffer += cp_size;
		size -= cp_size;
		write_block(block_id,block_offset,size,buf);
		start += 1;
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
	SBFS_read(parent_inum,(node.size-sizeof(dir)),&new_dir);
	strcpy(new_dir.filename,filename);
	new_dir.inum = inum;
	SBFS_write(parent_inum,(node.size-sizeof(dir)),&new_dir);
	strcpy(new_dir.filename,"");
	new_dir.inum = 0;
	SBFS_write(parent_inum,node.size,&new_dir);

	return inum;
}

int SBFS_mknod(char* path,char* filename){
	uint64_t inum = allocate_inode();
	int parent_inum = SBFS_namei(path);
	dir new_dir;
	SBFS_read(parent_inum,(node.size-sizeof(dir)),&new_dir);
	strcpy(new_dir.filename,filename);
	new_dir.inum = inum;
	SBFS_write(parent_inum,(node.size-sizeof(dir)),&new_dir);
	new_dir.inum = 0;
	SBFS_write(parent_inum,node.size,&new_dir);
	return inum;
}

int SBFS_unlink(char* path){
	inode node;
	uint64_t inum = SBFS_namei(path);
	read_inode(inum,&node);
	char data[BLOCKSIZE];
	dir* dirs = (dir*) data;
	int offset = 0;
	if(node->type == DIRECTORY){
		while(SBFS_read(inum,offset,50*sizeof(dir),data)!=EOF){
			for(int i=0;i<50;i++){
				if(dirs[i].inum==0)
					break;
				if(dirs[i].valid==1)
					free_inode(inum);
			}
		}
	}
	free_inode(inum);
	return 0;
}

int SBFS_close(int node){
	return 0;
}

uint64_t SBFS_open(char* filename,int mode){
	uint64_t inode = SBFS_namei(filename);
	return inode;
}

int SBFS_init(){
	mkfs();
}


void SBFS_readdir(){

}