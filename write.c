#include <stdio.h>
int main(){
    FILE* fp = fopen("vdc","w+");
    fwrite("123123123",sizeof(char),20,fp);
    fclose(fp);
    return 0;
}