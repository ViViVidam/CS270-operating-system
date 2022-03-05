//
// Created by xie on 2022/3/1.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
int main(){
    struct stat buf;

    stat("mount/link",&buf);
    printf("%d %d %d %x %x %x\n", S_ISLNK(buf.st_mode),S_ISREG(buf.st_mode), S_ISDIR(buf.st_mode),buf.st_mode,buf.st_mode&S_IFMT,S_IFMT);
    printf("%d\n",buf.st_size);
    //lstat("mount/link",&buf);
    //printf("%d %d %x %x %x\n", S_ISLNK(buf.st_mode),S_ISREG(buf.st_mode),buf.st_mode,buf.st_mode&S_IFMT,S_IFMT);
    //printf("%d\n",buf.st_size);
    return 0;
}