#include <stdint.h>
#define BLOCKSIZE  4096 //4K
#define BLOCKCOUNT  100

/*typedef struct
{
	uint8_t data[BLOCKSIZE];	
} block;*/

static uint8_t dev[BLOCKCOUNT][BLOCKSIZE];
int write_disk(uint64_t block_id,void* buffer);
int read_disk(uint64_t block_id,void* buffer);