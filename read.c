//
// Created by xie on 2022/3/7.
//

#include <stdio.h>
int main(){
    FILE* fp = fopen("vdc","w+");
    const char buf[20];
    fread(buf,sizeof(char),20,fp);
    fclose(fp);
    printf("%s\n",buf);
    return 0;
}