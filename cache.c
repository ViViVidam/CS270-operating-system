//
// Created by xie on 2022/3/6.
//
#include "disk.h"
#include "cache.h"

#define INDEXMASK 0x
static uint8_t cache[CACHESIZE][BLOCKSIZE];