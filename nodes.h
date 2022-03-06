#ifndef CS270_NODE_H
#define CS270_NODE_H
#include <stdint.h>

#define BLOCKADDR 8
#define DIRECT_BLOCK 10
#define SING_INDIR 1
#define DOUB_INDIR 1
#define TRIP_INDIR 1

#define FILEMASK 0x7000
#define FLAGMASK 0x8000
#define USERBMASK 0x0800
#define GROUPBMASK 0x0400
#define STICKBIT 0x0200
#define OWNERMASK 0x01c0
#define GROUPMASK 0x38
#define WORLDMASK 0x7

enum {
    NORMAL, DIR, SYMBOLIC, BLOCKDEVICE, CHARDEVICE, SOCK, PIPE
};// -,d,l,b,c,s,p

#define ISFILE(a) (((a) & FILEMASK) >> 12 == NORMAL)
#define ISSYM(a)  (((a) & FILEMASK) >> 12 == SYMBOLIC)
#define ISDIR(a)  (((a) & FILEMASK) >> 12 == DIR)
//assert
typedef struct { 
	//uint64_t blocks[13];
	
	uint64_t direct_blocks[DIRECT_BLOCK];//8*10
	uint64_t sing_indirect_blocks[SING_INDIR];//88
	uint64_t doub_indirect_blocks[DOUB_INDIR];//96
	uint64_t trip_indirect_blocks[TRIP_INDIR];//104
	uint64_t size;//128
    uint64_t last_access_time;
    uint64_t last_modify_time;
    uint16_t link;//hard link count
    uint16_t owner;
    uint16_t group;
    uint16_t permission_bits;
    //flag, FILETYPE(3 bits), set user bit, set group ID bit, sticky bit, 9-bits
} inode;

int write_block(uint64_t block_id,uint64_t offset,uint64_t size,void* buffer);
int read_block(uint64_t block_id,uint64_t offset,uint64_t size,void* buffer);


void read_inode(uint64_t inum,inode* node);
void write_inode(uint64_t inum,inode* node);
int free_inode(uint64_t inum);
uint64_t allocate_inode();

int free_data_block(uint64_t id);
uint64_t allocate_data_block();
void mkfs();

uint64_t read_head();//debuging purpose

#endif