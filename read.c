#include <stdio.h>
#include <string.h>
int main(int argc,char *argv[]){
    FILE* fp = fopen(argv[1],"w+");
    char buf[512];
    if(fp!=NULL) {
        fread(buf,sizeof(char), 512,fp);
        printf("read from file: %s\n",buf);
        fclose(fp);
    }
    else
        printf("fp NULL\n");
    return 0;
}