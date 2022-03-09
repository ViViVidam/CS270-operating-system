#include <stdio.h>
#include <string.h>
int main(int argc,char *argv[]){
    FILE* fp = fopen(argv[1],"w+");

    if(fp!=NULL) {
        fwrite(argv[2],sizeof(char), strlen(argv[3]),fp);
        fclose(fp);
    }
    else
        printf("fp NULL\n");
    return 0;
}