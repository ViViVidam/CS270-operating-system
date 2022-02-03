#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char* argv[]){
    if(argc!=3)
        printf("wrong format, usage:\n./test string offset\n");
    char buf[4096];
    char filename[40]= "./mount/a";
    int f_write= open(filename,O_WRONLY);
    write(f_write,argv[1],strlen(argv[1]));
    int f_read = open(filename, O_RDONLY);
    lseek(f_read,atoi(argv[2]),SEEK_CUR);
    read(f_read,buf,strlen(argv[1]));
    printf("%s\n",buf);
}