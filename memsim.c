#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

int global_time_accessed = 0; // DELETE ME testing

typedef struct PTE {
    char VA[8 + 1];
    int int_VA;

    int valid;
    int present;
    int dirty;
    int time_accessed;

    struct PTE *prevPTE;
    struct PTE *nextPTE;
} PTE;

typedef struct dequeue {
    PTE *front;
    PTE *rear;
    int size;
    int capacity;
} dequeue;

void init_dequeue(dequeue *dq, int max_size);
int dequeue_empty(dequeue *dq);
int dequeue_full(dequeue *dq);
int insert_front_dequeue(dequeue *dq, PTE entry);
int insert_rear_dequeue(dequeue *dq, PTE entry);
void print_dequeue(dequeue *dq);

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
    
    dequeue Q;
    init_dequeue(&Q, nframes);

    print_dequeue(&Q);

    int events_ctr = 0;
    PTE newPTE;
    while( fscanf(trace_file, "%s %c", virtual_addr, &op_type) != EOF){
        // Size plus termination character for each part of the virtual address
        char vpn[3 + 1];
        char offset[5 + 1];

        // Copies relevant parts of the virtual address into two variables
        strncpy(vpn, virtual_addr, 3);
        strncpy(offset, virtual_addr + 3, 5);

        vpn[3] = '\0';
        offset[5] = '\0';

        printf("%s %c\n", vpn, op_type);
        printf("%s %c\n", offset, op_type);
        printf("%s\n", virtual_addr);

        if (++events_ctr < 300) { // DELETE ME

            newPTE.int_VA = atoi(virtual_addr);

            if (!PTE_present(&Q, newPTE))
                insert_rear_dequeue(&Q, newPTE);

        }
        else {
            break;
        }
    }

    printf("events in trace: %d\n", events_ctr);

    print_dequeue(&Q);
    // Closes trace file
    fclose(trace_file);
    return 0;
}

void init_dequeue(dequeue *dq, int max_size) {
    dq->front = NULL;
    dq->rear = NULL;
    dq->size = 0;
    dq->capacity = max_size;
}

int dequeue_empty(dequeue *dq) {
    return dq->size == 0;
}

int dequeue_full(dequeue *dq) {
    return dq->size == dq->capacity;
}

int insert_front_dequeue(dequeue *dq, PTE entry) {
    if (dq->size == dq->capacity)
        return -1;

    PTE *newPTE = (PTE*) malloc(sizeof(struct PTE));
    newPTE->nextPTE = NULL;
    newPTE->prevPTE = NULL;

    if (newPTE != NULL) {
        newPTE->time_accessed = ++global_time_accessed; // DELETE ME
        newPTE->int_VA = entry.int_VA;

        if (dq->front == NULL) {
            dq->front = newPTE;
            dq->rear = newPTE;
        }
        else {
            newPTE->nextPTE = dq->front;
            dq->front->prevPTE = newPTE;
            dq->front = newPTE;
        }
    } 
    else {
        return -1;
    }

    dq->size++;

    return dq->size;
}

int insert_rear_dequeue(dequeue *dq, PTE entry) {
    if (dq->size == dq->capacity)
        return -1;

    PTE *newPTE = (PTE*) malloc(sizeof(struct PTE));
    newPTE->nextPTE = NULL;
    newPTE->prevPTE = NULL;


    if (newPTE != NULL) {
        newPTE->time_accessed = ++global_time_accessed; // DELETE ME
        newPTE->int_VA = entry.int_VA;

        if (dq->rear == NULL) {    
            dq->rear = newPTE;
            dq->front = newPTE;
        }
        else {
            newPTE->nextPTE = dq->rear;
            dq->rear->prevPTE = newPTE;
            dq->rear = newPTE;
        }
    }
    else {
        return -1;
    }

    dq->size++;

    return dq->size;
}

int PTE_present(dequeue *dq, PTE entry) {
    PTE *iter = dq->rear;
    int present = 0;

    if (dq->size == 0) {
        printf("The dequeue is empty.\n");
    }
    else {
        while (iter != NULL) {
            if (iter->int_VA == entry.int_VA) {
                present = 1;
                break;
            }
            iter = iter->nextPTE;
        }
    }

    return present;
}

void print_dequeue(dequeue *dq) {
    PTE *iter = dq->rear;

    if (dq->size == 0) {
        printf("The dequeue is empty.\n");
    }
    else {
        while (iter != NULL) {
            printf("This is PTE #%d: %d\n", iter->time_accessed, iter->int_VA);
            iter = iter->nextPTE;
        }
    }
}