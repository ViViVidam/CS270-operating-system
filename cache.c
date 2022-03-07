//
// Created by xie on 2022/3/6.
//
#include "disk.h"
#include "cache.h"

#define INDEXMASK 0xF;
static uint8_t cache[CACHESIZE][2][BLOCKSIZE];
static uint8_t flag[CACHESIZE][2];
static uint64_t group[CACHESIZE][2];
int readCache(const char* buf,unsigned long blockId){
    uint8_t index = blockId & 0xf;
    uint64_t identity = blockId & ~(0xf);
    for()
    if(flag[index][0] == 1 && identity)
}
int writeCache(const char* buf,unsigned long blockId){

}