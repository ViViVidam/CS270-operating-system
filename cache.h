//
// Created by xie on 2022/3/6.
//

#ifndef CS270_CACHE_H
#define CS270_CACHE_H

#define CACHESIZE 16 //changing the cachesize must also change the mask
#define GROUPSIZE 2
int readCache(const char* buf,unsigned long blockId);
int writeCache(const char* buf,unsigned long blockId);
#endif //CS270_CACHE_H
