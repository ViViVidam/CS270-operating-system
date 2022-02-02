#include <stdio.h>

int main(){
    char buf[4096];
    FILE * fp= fopen("mount/hello","r");
    while(fscanf(fp,"%s",buf)!=EOF)
        printf("%s\n",buf);
}