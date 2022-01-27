#include <stdint.h>
#define BLOCKADDR 8
#define DIRECT_BLOCK 10
#define SING_INDIR 1
#define DOUB_INDIR 1
#define TRIP_INDIR 1

typedef struct { 
	uint64_t direct_blocks[DIRECT_BLOCK];//8*10
	uint64_t sing_indirect_blocks[SING_INDIR];//88
	uint64_t doub_indirect_blocks[DOUB_INDIR];//96
	uint64_t trip_indirect_blocks[TRIP_INDIR];//104
	uint64_t count;//112
	uint64_t owner;//120
} inode;

uint64_t head = 0;

int write_block(unsigned int block_id,unsigned int offset,unsigned int size,void* buffer);
int read_block(unsigned int block_id,unsigned int offset,unsigned int size,void* buffer);
int free_data_block(int id);
uint64_t allocate_data_block();
uint64_t allocate_data_block();