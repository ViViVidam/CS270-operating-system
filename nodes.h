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

int head = 0;