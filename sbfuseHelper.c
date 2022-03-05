#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sbFuseHelper.h"
void getCompletePath(char* path,char* complete_path,unsigned int pid) {
    char prefix[50] = "/proc/";
    char end[10] = "/cwd";
    char buf[200];
    sprintf(buf, "%d", pid);
    strcat(prefix, buf);
    strcat(prefix, end);
    readlink(prefix, buf, 200);
    unsigned int length = strlen(buf);
    buf[length] = '/';
    length += 1;
    deduplicate(path);
    int j = 0;
    printf("%s\n",buf);
    for (int i = 0; i < strlen(path); i++) {
        printf("%c\n",path[i]);
        if (path[i] == '/') {
            if (j != 2) {
                buf[length + j] = path[i];
                length += (j + 1);
                j = 0;
                continue;
            }
            if (buf[length + 1] == '.' && buf[length] == '.') {
                for (int k = (length - 2); k > 0; k--) {
                    if (buf[k] == '/') {
                        length = k + 1;
                        break;
                    }
                }//k should not be zero here
            } else {
                buf[length + j] = path[i];
                length += (j + 1);
            }
            j = 0;
        } else {
            buf[length + j] = path[i];
            j++;
        }
    }
    length += j;
    buf[length] = 0;
    strcpy(complete_path, buf);
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