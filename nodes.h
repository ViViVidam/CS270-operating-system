#include <stdint.h>
#define BLOCKADDR 8
#define DIRECT_BLOCK 10
#define SING_INDIR 1
#define DOUB_INDIR 1
#define TRIP_INDIR 1
#define ROOT 0

typedef struct { 
	uint64_t direct_blocks[DIRECT_BLOCK];//8*10
	uint64_t sing_indirect_blocks[SING_INDIR];//88
	uint64_t doub_indirect_blocks[DOUB_INDIR];//96
	uint64_t trip_indirect_blocks[TRIP_INDIR];//104
	uint64_t count;//112
	uint64_t flag;//120 this is set to 64bit because of alignment
} inode;

static uint64_t head = 0;
static uint64_t i_list_size = 0;

void write_block(inode* node,uint64_t index,unsigned int size,void* buf);
int read_block(inode* node,uint64_t index,unsigned int size,void* buf);
int free_data_block(int id);
uint64_t allocate_data_block();
uint64_t allocate_data_block();
inode* get_inode(uint64_t inum);
int allocate_inode();
int free_inode(uint64_t inum);