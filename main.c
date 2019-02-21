#include <stdio.h>

int main(){
    FILE *file;
    file = fopen("bash.trace", "r");
    unsigned addr;
    char rw;
    fscanf(file,"%x %c",&addr,&rw);
    return 0;
}