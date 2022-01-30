#include "disk.h"
#include <stdint.h>

#define BLOCKADDR 8
#define DIRECT_BLOCK 10
#define SING_INDIR 1
#define DOUB_INDIR 1
#define TRIP_INDIR 1
//assert
typedef struct { 
	//uint64_t blocks[13];
	
	uint64_t direct_blocks[DIRECT_BLOCK];//8*10
	uint64_t sing_indirect_blocks[SING_INDIR];//88
	uint64_t doub_indirect_blocks[DOUB_INDIR];//96
	uint64_t trip_indirect_blocks[TRIP_INDIR];//104
	uint64_t type;//112
	uint64_t flag;//120 this is set to 64bit because of alignment
	uint64_t size;//128
} inode;

int write_block(uint64_t block_id,uint64_t offset,uint64_t size,void* buffer);
int read_block(uint64_t block_id,uint64_t offset,uint64_t size,void* buffer);


void read_inode(uint64_t inum,inode* node);
void write_inode(uint64_t inum,inode* node);

int free_data_block(uint64_t id);
int free_inode(uint64_t inum);
uint64_t allocate_inode();
uint64_t allocate_data_block();
void mkfs();

uint64_t read_head();//debuging purpose