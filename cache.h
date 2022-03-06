//
// Created by xie on 2022/3/6.
//

#ifndef CS270_CACHE_H
#define CS270_CACHE_H

#define CACHESIZE 16 //changing the cachesize must also change the mask
int readCache(char* buf,unsigned long blockId);
int writeCache(char* buf,unsigned long blockId);
#endif //CS270_CACHE_H
