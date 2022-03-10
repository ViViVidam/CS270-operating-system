#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "nodes.h"
#define INDEXMASK 0xF;
#define INODE_PER_BLOCK (BLOCKSIZE / sizeof(inode))

static uint8_t cache[CACHESIZE][GROUPSIZE][BLOCKSIZE];
static uint8_t flag[CACHESIZE][GROUPSIZE];
static uint8_t dirty[CACHESIZE][GROUPSIZE];
static uint64_t identities[CACHESIZE][GROUPSIZE];
static uint8_t timestamp[CACHESIZE][GROUPSIZE];
static uint64_t head = 0;
static uint64_t i_list_block_count = 0;
/* the entire inode is loaded into memory, mapping from inum to index is inum - 1 = index*/
static inode* in_mem_ilist;
static uint64_t last_allocated = 1;

void cp_inode(inode *dest, inode *source)
{
    for (int i = 0; i < (DIRECT_BLOCK);i++){
        dest->direct_blocks[i] = source->direct_blocks[i];
    }
    for (int i = 0; i < (SING_INDIR);i++){
        dest->sing_indirect_blocks[i] = source->sing_indirect_blocks[i];
    }
    for (int i = 0; i < (DOUB_INDIR);i++){
        dest->doub_indirect_blocks[i] = source->doub_indirect_blocks[i];
    }
    for (int i = 0; i < (TRIP_INDIR);i++){
        dest->trip_indirect_blocks[i] = source->trip_indirect_blocks[i];
    }
    dest->permission_bits = source->permission_bits;
    dest->size = source->size;
    dest->last_access_time = source->last_access_time;
    dest->last_modify_time = source->last_modify_time;
    dest->owner = source->owner;
    dest->link = source->link;
    dest->link = source->link;
}

void write_inode(uint64_t inum, inode *node)
{
    char data[BLOCKSIZE];
    //printf("write %ld %ld %ld\n",node->size,node->sing_indirect_blocks[0],node->flag);
    uint64_t block_id = (inum - 1) / INODE_PER_BLOCK + 1;
    uint64_t offset = (inum - 1) % INODE_PER_BLOCK;
    read_disk(block_id, data);
    inode *inodes = (inode *)data;
    assert((node->permission_bits & FLAGMASK) == 0x8000);
    /* length is used to escape zero out the memory when allocate inode in user space */
    cp_inode(&inodes[offset], node);
    cp_inode(in_mem_ilist + (inum - 1),node);
    write_disk(block_id, data);
}

void init_free_disk(int start)
{
    char data[BLOCKSIZE];
    char test[BLOCKSIZE];
    int block_entries_per_block = BLOCKSIZE / BLOCKADDR; // num of addr of block per block
    int record_block_count = (BLOCKCOUNT - i_list_block_count - 1) / block_entries_per_block;
    uint64_t *datablock = (uint64_t *)data;
    int block_id = start;

    head = start;

    while (record_block_count--)
    {
        block_id = start;
        for (int i = 1; i < block_entries_per_block; i++)
        {
            start++;
            datablock[i] = start;
        }
        start++;
        datablock[0] = start;
        write_disk(block_id, datablock);
        read_disk(block_id,test);
    }

    if ((BLOCKCOUNT - i_list_block_count - 1) % block_entries_per_block != 0)
    {
        int block_id = start;
        memset(data, 0, BLOCKSIZE);
        for (int i = 1; i < block_entries_per_block; i++)
        {
            start++;
            if (start > (BLOCKCOUNT - 1))
                datablock[i] = 0;
            else
                datablock[i] = start;
        }
        datablock[0] = 0;
        write_disk(block_id, datablock);
        read_disk(block_id,test);
        uint64_t * test_block = (uint64_t*) test;
        /*printf("block id %d:\n",block_id);
        for(int j = 0;j<(BLOCKSIZE/BLOCKADDR);j++){
            printf("%ld ",test_block[j]);
        }
        printf("\n");*/
    }
    else
    {
        datablock[0] = 0;
        write_disk(block_id, datablock); //rewrite the last block
        read_disk(block_id,test);
        uint64_t * test_block = (uint64_t*) test;
        /*printf("block id %d:\n",block_id);
        for(int j = 0;j<(BLOCKSIZE/BLOCKADDR);j++){
            printf("%ld ",test_block[j]);
        }
        printf("\n");*/
    }
}

void init_i_list()
{
    char tmp[BLOCKSIZE];
    memset(tmp, 0, sizeof(tmp));
    i_list_block_count = BLOCKCOUNT / 10;
    for (int i = 1; i <= i_list_block_count; i++)
    {
        write_disk(i, tmp);
    }
    in_mem_ilist = (inode*)malloc(INODE_PER_BLOCK*i_list_block_count*sizeof(inode));
    memset(in_mem_ilist,0,INODE_PER_BLOCK*i_list_block_count*sizeof(inode));
}

int mkfs()
{
    char tmp[BLOCKSIZE];
    read_disk(0,tmp);
    uint64_t *supernode = (uint64_t *)tmp;
    if(supernode[3] != 1) {
        memset(tmp, 0, sizeof(tmp));
        init_i_list();
        //int start = INODES / INODE_PER_BLOCK + 1;
        int start = i_list_block_count + 1;
        //start of block
        supernode[0] = start;
        supernode[1] = i_list_block_count; //inode block count
        supernode[2] = BLOCKCOUNT;
        supernode[3] = 1; //init flag
        write_disk(0, tmp);

        printf("data block starts from block %d\n", start);
        init_free_disk(start);
        return 0;
    }
    else{
        i_list_block_count = supernode[1];
        head = supernode[0];
        in_mem_ilist = (inode*)malloc(INODE_PER_BLOCK*i_list_block_count*sizeof(inode));
        inode* nodes = (inode*)tmp;
        for(int i = 1; i<= i_list_block_count;i+=1){
            read_disk(i,tmp);
            for(int j=0;j<INODE_PER_BLOCK;j++){
                memcpy(in_mem_ilist+(i-1)*INODE_PER_BLOCK+j,nodes+j,sizeof(inode));
            }
        }
        return 1;
    }
    //printf("header %ld\n",head);
}

/*
 * inode count starts from 1,
 * this function used only for higher level
 * same level can read direct from in_mem_list
 * */
void read_inode(uint64_t inum, inode *node){
    cp_inode(node, in_mem_ilist + (inum-1) );
}

/* zero for failure */
uint64_t allocate_inode() {
    char tmp[BLOCKSIZE];
    int i, flag = 0;
    for (i = last_allocated; i <= i_list_block_count*INODE_PER_BLOCK; i++){
        if( (in_mem_ilist[i-1].permission_bits & FLAGMASK) == 0){
            int block_id = (i - 1) / INODE_PER_BLOCK;
            int offset = (i - 1) % INODE_PER_BLOCK;
            read_disk(block_id,tmp);
            inode* inodes = (inode*) tmp;
            inodes[offset].permission_bits = inodes[offset].permission_bits | (1 << 15);
            inodes[offset].link = 1;
            in_mem_ilist[i-1].permission_bits = in_mem_ilist[i-1].permission_bits | (1 << 15);
            in_mem_ilist[i-1].link = 1;
            last_allocated = i + 1;
            write_disk(block_id,tmp);
            flag = 1;
            break;
        }
    }
    if(!flag){
        printf("inode full\n");
        i = 0;
    }
    return i;
}

int free_inode(uint64_t inum) {
    if(inum == 0)
        return -1;
    uint64_t block_id = 1 + (inum - 1) / INODE_PER_BLOCK;
    uint32_t offset = (inum - 1) % INODE_PER_BLOCK;
    memset(in_mem_ilist + (inum - 1),0,sizeof(inode));
    inode tmp;
    if (block_id > i_list_block_count)
    {
        printf("free block_id too big %ld \n", inum);
        return -1;
    }
    read_block(block_id, sizeof(inode) * offset, sizeof(inode), &tmp);

    for (int i = 0; i < DIRECT_BLOCK; i++)
    {
        if(tmp.direct_blocks[i]) {
            free_data_block(tmp.direct_blocks[i]);
            tmp.direct_blocks[i] = 0;
        }
        else
            break;
    }
    for (int i = 0; i < SING_INDIR; i++)
    {
        if(tmp.sing_indirect_blocks[i]) {
            free_data_block(tmp.sing_indirect_blocks[i] = 0);
            tmp.sing_indirect_blocks[i] = 0;
        }
        else
            break;
    }
    for (int i = 0; i < DOUB_INDIR; i++){
        if(tmp.doub_indirect_blocks[i]) {
            free_data_block(tmp.doub_indirect_blocks[i] = 0);
            tmp.doub_indirect_blocks[i] = 0;
        }
        else
            break;
    }
    for (int i = 0; i < TRIP_INDIR; i++){
        if(tmp.trip_indirect_blocks[i]) {
            free_data_block(tmp.trip_indirect_blocks[i] = 0);
            tmp.trip_indirect_blocks[i] = 0;
        }
        else
            break;
    }
    tmp.last_modify_time = 0;
    tmp.last_access_time = 0;
    tmp.owner =0;
    tmp.permission_bits = 0;
    tmp.size = 0;
    write_block(block_id, sizeof(inode) * offset, sizeof(inode), &tmp);
    last_allocated = inum;
    return 0;
}

uint64_t allocate_data_block() {
    uint8_t tmp[BLOCKSIZE];
    uint64_t *data = (uint64_t *)tmp;
    read_block(head, 0, BLOCKSIZE, tmp);
    int i = 0;
    uint64_t res = 0;
    for (i = 1; i < (BLOCKSIZE / BLOCKADDR); i++)
    {
        if (data[i] != 0)
        {
            res = 1;
            break;
        }
    }
    if (res == 1)
    {
        res = data[i];
        data[i] = 0;
        write_disk(head, tmp);
    }
    else
    {
        res = head;
        head = data[0];
        data[0] = 0;
        write_block(0, 0, sizeof(uint64_t), &head);
        write_disk(res, tmp);
    }
    printf("block allocated %ld\n",res);
    return res;
}

int free_data_block(uint64_t id)
{
    uint8_t tmp[BLOCKSIZE];
    uint64_t *data = (uint64_t *)tmp;
    read_block(head, 0, BLOCKSIZE, tmp);
    int i = 0, flag = 0;
    for (i = 1; i < (BLOCKSIZE / BLOCKADDR); i++)
    {
        if (data[i] == 0)
        {
            flag = 1;
            data[i] = id;
            break;
        }
    }

    if (flag == 1)
        write_disk(head, tmp);
    else
    {
        uint64_t pre_head = head;
        printf("pre head %ld\n",pre_head);
        head = id;
        write_block(pre_head, 0, sizeof(uint64_t), &id);
    }
    printf("block freed %ld\n",id);
    return 0;
}

/* when to write back is a serious question */
int read_block(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
    char tmp[BLOCKSIZE];

    if (offset > BLOCKSIZE)
    {
        printf("offset %ld in write_disk greate than BLOCKSIZE\n", offset);
        return -1;
    }

    read_disk(block_id, tmp);
    memcpy((char *)buffer, tmp + offset, fmin(size, BLOCKSIZE - offset));

    return fmin(size, BLOCKSIZE - offset);
}

int write_block(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
    char tmp[BLOCKSIZE];

    if (offset > BLOCKSIZE)
    {
        printf("boundary exceeded %ld in write_disk greate than BLOCKSIZE\n", offset);
        return -1;
    }

    read_disk(block_id, tmp);
    memcpy(tmp + offset, (char *)buffer, fmin(size, BLOCKSIZE - offset));

    write_disk(block_id, tmp);

    return fmin(size, BLOCKSIZE - offset);
}

void cache_update_timestamp(int index) {
    for (int i = 0; i < GROUPSIZE; i++) {
        if (flag[index][i] == 1)
            timestamp[index][i] += 1;
    }
}
/**
 * read block return the size of read in actual
 * size can be greater than BLOCKSIZE, but it will be truncate
 * **/
int read_block_cache(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer) {
    assert(offset<=BLOCKSIZE);
    int read_size = 0;
    if(size<(BLOCKSIZE-offset))
        read_size = size;
    else
        read_size = BLOCKSIZE - offset;
    uint8_t index = block_id & INDEXMASK;
    uint64_t identity = block_id;
    cache_update_timestamp(index);
    for (int i = 0; i < GROUPSIZE; i++) {
        if (flag[index][i] == 1 & identities[index][i] == identity) {
            memcpy(buffer, cache[index][i] + offset, read_size);
            timestamp[index][i] = 0;
            return read_size;
        }
    }
    int kick_index = 0;
    for (int i = 0; i < GROUPSIZE; i++) {
        if(flag[index][i]==0) {
            kick_index = i;
            break;
        }
        else if(timestamp[index][kick_index] <= timestamp[index][i])
            kick_index = i;
    }

    if(dirty[index][kick_index]==1) {
        write_disk(identities[index][kick_index], cache[index][kick_index]);
        dirty[index][kick_index] = 0;
    }
    flag[index][kick_index] = 1;
    identities[index][kick_index] = block_id;
    timestamp[index][kick_index] = 0;
    read_disk(block_id,cache[index][kick_index]);
    memcpy(buffer,cache[index][kick_index]+offset,read_size);
    printf("\nread block cache %ld,offset %ld size %ld buffer %s\n",block_id,offset,size,buffer);
    return read_size;
}

int write_block_cache(uint64_t block_id, uint64_t offset, uint64_t size, void *buffer)
{
    printf("\nwrite block cache %ld,offset %ld size %ld buffer %s\n",block_id,offset,size,buffer);
    assert(offset<BLOCKSIZE);
    int index = block_id & INDEXMASK;
    uint64_t identity = block_id;
    int write_size = 0;
    if(size<(BLOCKSIZE-offset))
        write_size = size;
    else
        write_size = BLOCKSIZE - offset;

    cache_update_timestamp(index);

    for(int i = 0;i<GROUPSIZE;i++){
        if(flag[index][i] == 1 && identities[index][i] == identity){
            memcpy(cache[index][i]+offset,buffer,write_size);
            timestamp[index][i] = 0;
            dirty[index][i] = 1;
            return write_size;
        }
    }

    int kick_index = 0;
    for (int i = 0; i < GROUPSIZE; i++) {
        if(flag[index][i]==0) {
            kick_index = i;
            break;
        }
        else if(timestamp[index][kick_index] <= timestamp[index][i])
            kick_index = i;
    }

    if(dirty[index][kick_index]==1)
        write_disk(identities[index][kick_index], cache[index][kick_index]);

    flag[index][kick_index] = 1;
    dirty[index][kick_index] = 1;
    identities[index][kick_index] = block_id;
    timestamp[index][kick_index] = 0;
    read_disk(block_id,cache[index][kick_index]);
    memcpy(cache[index][kick_index]+offset,buffer,write_size);

    return write_size;
}


void cache_flush_all(){
    for(int i=0;i<CACHESIZE;i++) {
        for (int j = 0; j < GROUPSIZE; j++) {
            if (flag[i][j] == 1 && dirty[i][j] == 1) {
                write_disk(identities[i][j], cache[i][j]);
                dirty[i][j] = 0;
            }
        }
    }
}
void cache_flush(uint64_t id) {
    int index = id & INDEXMASK;
    for (int i = 0; i < GROUPSIZE; i++) {
        if (flag[index][i] == 1 && dirty[index][i] == 1 && identities[index][i] == id) {
            write_disk(id, cache[index][i]);
            break;
        }
    }
}
/* size is for computation convinent */