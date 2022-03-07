#include <stdio.h>
int main(){
    FILE* fp = fopen("mount/test","r");
    if(fp!=NULL)
        fclose(fp);
    return 0;
}