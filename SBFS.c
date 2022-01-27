#include "nodes.h"
#include "SBSF.h"
#include <stdio.h>
#include <string.h>

#define MAX_OPEN 100
#define DIR 0
#define FILE 1

typedef struct{
	char filename[MAX_FILENAME];
	uint64_t inum;
} dir;

file_describ open_file_table[MAX_OPEN];
inode_describ ilist[INODES];

int get_inode_node(inode* node,uint64_t index,void* buf){
	if(index<DIRECT_BLOCK){
		buf = get_inode()
	}
}

uint64_t find_file(inode* node, char* filename){

}

uint64_t namei(char* path){
	inode* tmp;
	char buf[BLOCKSIZE];
	char filename[MAX_FILENAME];
	char* pointer = path;
	int i = 0;
	uint64_t block_id;
	if(pointer[0]=='/'){
		tmp = get_inode(ROOT);
		pointer += 1;
	}
	while( pointer!= 0){ 
		if(*pointer != '/'){
			filename[i++] = *pointer;
			pointer += 1; 
		}
		filename[i] = 0;
		read_block()
	}
}

int SBFS_read(){

}

int SBFS_mkdir(char* path, char* filename){
	uint64_t inum = allocate_inode();
	return inum;
}



int SBFS_open(char* filename,int mode){
	uint64_t inode = namei(filename);
}

int SBFS_init(){
	mkfs();
}