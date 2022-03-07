#include <stdio.h>
int main(){
    FILE* fp = fopen("/dev/vdc","w+");
    fwrite("123123123",sizeof(char),20,fp);
    fclose(fp);
    return 0;
}