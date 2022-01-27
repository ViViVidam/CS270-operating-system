#include "nodes.h"

#define MAX_OPEN 100
#define DIR 0
#define FILE 1

typedef struct{
	uint8_t type;
} file;

file_describ open_file_table[MAX_OPEN];
inode_describ ilist[INODES];

int mkdir(char* path, char* filename){

}

int namei(char* filename){

}

int open(char* filename,int mode){
	
}