//
// Created by xie on 2022/3/6.
//
#include "disk.h"
#include "cache.h"
#include <string.h>
#define INDEXMASK 0xF;
static uint8_t cache[CACHESIZE][GROUPSIZE][BLOCKSIZE];
static uint8_t flag[CACHESIZE][GROUPSIZE];
static uint8_t dirty[CACHESIZE][GROUPSIZE];
static uint64_t group[CACHESIZE][GROUPSIZE];
int readCache(const char* buf,unsigned long blockId) {
    uint8_t index = blockId & 0xf;
    uint64_t identity = blockId & ~(0xf);
    for (int i = 0; i < GROUPSIZE; i++) {
        if(flag[index][i]==1 & group[index][i] == identity){
            memcpy()
        }

    }
    if (flag[index][0] == 1 && identity)
}
int writeCache(const char* buf,unsigned long blockId){

}