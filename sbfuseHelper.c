#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sbFuseHelper.h"
void getCompletePath(char* path,char* complete_path,unsigned int pid){
    char prefix[50]="/proc/";
    char end[10] = "/cwd";
    char buf[200];
    sprintf(buf,"%d",pid);
    strcat(prefix,buf);
    strcat(prefix,end);
    readlink(prefix,buf,200);
    unsigned int length = strlen(buf);
    buf[length] = '/';
    buf[length+1] = 0;
    strcat(buf,path);
    deduplicate(buf);
    strcpy(complete_path,buf);
}

void deduplicate(char* buf) {
    int mode = 0;
    int i = 0,j=0;
    char cleaned[200];
    for (i = 0; i < strlen(buf); i++) {
        if(buf[i]!='/'){
            cleaned[j++] = buf[i];
            mode = 0;
        }
        if(buf[i]=='/'){
            if(mode==0) {
                mode = 1;
                cleaned[j++] = buf[i];
            }
        }
    }
    cleaned[j] = 0;
    strcpy(buf,cleaned);
}