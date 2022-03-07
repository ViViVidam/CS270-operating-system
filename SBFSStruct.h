//
// Created by xie on 2022/2/20.
//

#ifndef CS270_SBFSSTRUCT_H
#define CS270_SBFSSTRUCT_H

#include "nodes.h"

#define MAX_FILENAME 64
#define MAX_PATH 1024
#define ROOT 1
#define NOREPLACE (1 << 0)
#define EXCHANGE (1 << 1)

#define H_CREATE 1
#define H_READ 2

typedef struct
{
    char filename[MAX_FILENAME];
    uint64_t inum;
} dir;


#endif //CS270_SBFSSTRUCT_H
