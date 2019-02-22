#include <stdio.h>

struct PTE {
    int PA;
    int valid;
    int present;
    int dirty;
    int accessed;
};

int main(int argc, char *argv[]) {
    unsigned addr;
    char rw;

    FILE *trace_file = fopen(argv[1], "r");

    while( fscanf(trace_file, "%x %c", &addr, &rw) != EOF){
        printf("%x %c\n", addr, rw);
    }

    fclose(trace_file);
    return 0;
}
