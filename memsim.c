#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PTE {
    int PA;
    int valid;
    int present;
    int dirty;
    int accessed;
};

int main(int argc, char *argv[]) {
    // Input arguments
    char *input_file = argv[1];
    int nframes = atoi(argv[2]);
    char *algo = argv[3];
    char *mode = argv[4];

    // Opens trace file in read mode
    FILE *trace_file = fopen(input_file, "r");

    // Storage for variables on each line of the trace file
    char virtual_addr[8 + 1];
    char op_type;
    
    int events_ctr = 0;
    while( fscanf(trace_file, "%s %c", virtual_addr, &op_type) != EOF){
        // Size plus termination character for each part of the virtual address
        char vpn[3 + 1];
        char offset[5 + 1];

        // Copies relevant parts of the virtual address into two variables
        strncpy(vpn, virtual_addr, 3);
        strncpy(offset, virtual_addr + 3, 5);

        printf("%s %c\n", vpn, op_type);
        printf("%s %c\n", offset, op_type);

        if (++events_ctr < 10) { // DELETE ME
        }
        else {
            break;
        }
    }

    printf("events in trace: %d\n", events_ctr);

    // Closes trace file
    fclose(trace_file);
    return 0;
}
