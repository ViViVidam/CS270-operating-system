#include <stdio.h>
int main(int argc,char *argv[]){
    FILE* fp = fopen("mount/test","w+");
    if(fp!=NULL)
        fclose(fp);
    return 0;
}