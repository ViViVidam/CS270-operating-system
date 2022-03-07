//
// Created by xie on 2022/3/6.
//

#ifndef CS270_CACHE_H
#define CS270_CACHE_H


int readCache(const char* buf,unsigned long blockId);
int writeCache(const char* buf,unsigned long blockId);
#endif //CS270_CACHE_H
