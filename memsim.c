#include <stdio.h>
#include <string.h>

struct PTE {
    int PA;
    int valid;
    int present;
    int dirty;
    int accessed;
};

int main(int argc, char *argv[]) {
    char virtual_addr[8 + 1];
    char op_type;

    FILE *trace_file = fopen(argv[1], "r");

    int ctr = 0;// DELETE ME
    while( fscanf(trace_file, "%s %c", virtual_addr, &op_type) != EOF){
        char vpn[3 + 1];
        char offset[5 + 1];
        strncpy(vpn, virtual_addr, 3);
        strncpy(offset, virtual_addr + 3, 5);

        printf("%s %c\n", vpn, op_type);
        printf("%s %c\n", offset, op_type);

        if (++ctr >= 10) // DELETE ME
            break; 
    }

    fclose(trace_file);
    return 0;
}
