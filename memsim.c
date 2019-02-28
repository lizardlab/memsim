#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

int global_time_accessed = 0; // DELETE ME testing

int get_access_time() {
    return ++global_time_accessed;
}

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

enum algo_types{LRU, FIFO, VMS};
enum mode_types{QUIET, DEBUG};


void init_dequeue(dequeue *dq, int max_size);
int dequeue_empty(dequeue *dq);
int dequeue_full(dequeue *dq);
int insert_front_dequeue(dequeue *dq, PTE entry);
int insert_rear_dequeue(dequeue *dq, PTE entry);
int remove_front_dequeue(dequeue *dq);
int remove_rear_dequeue(dequeue *dq);
PTE *replace_PTE(dequeue *dq, PTE *victim, PTE entry);
void print_dequeue(dequeue *dq);
PTE *PTE_present(dequeue *dq, PTE entry); // maybe return pointer to entry for use in replace PTE
PTE *find_LRU(dequeue *dq);

int main(int argc, char *argv[]) {
    // Input arguments
    char *input_file = argv[1];
    int nframes = atoi(argv[2]);
    char *algo = argv[3];
    char *mode = argv[4];

    // Opens trace file in read mode
    FILE *trace_file = fopen(input_file, "r");

    if (trace_file == NULL) {
        printf("Failed to open input file.");
        return -1;
    }
    
    if (nframes <= 0) {
        printf("Invalid number of frames.\n");
        return -1;
    }

    enum algo_types replace_with;
    enum mode_types running_mode;

    if (strcmp(algo, "lru") == 0) {
        replace_with = LRU;
    } else if (strcmp(algo, "fifo") == 0) {
        replace_with = FIFO;
    }  else if (strcmp(algo, "vms") == 0) {
        replace_with = VMS;
    } else {
        printf("Algorithm not recognized.\n");
        return -1;
    }

    if (strcmp(mode, "quiet") == 0) {
        running_mode = QUIET;
    } else if (strcmp(mode, "debug") == 0) {
        running_mode = DEBUG;
    }  else {
        printf("Mode not recognized.\n");
        return -1;
    }


    // Storage for variables on each line of the trace file
    char virtual_addr[8 + 1];
    char op_type;
    dequeue Q;
    init_dequeue(&Q, nframes);

    int events_ctr = 0;
    int page_faults_ctr = 0;
    int disk_reads_ctr = 0;
    int disk_writes_ctr = 0;
    PTE newPTE;

    while( fscanf(trace_file, "%s %c", virtual_addr, &op_type) != EOF){
        if (op_type == 'R')
            ++disk_reads_ctr;

        if (op_type == 'W')
            ++disk_writes_ctr;

        // Size plus termination character for each part of the virtual address
        char vpn[3 + 1];
        char offset[5 + 1];

        // Copies relevant parts of the virtual address into two variables
        strncpy(vpn, virtual_addr, 3);
        strncpy(offset, virtual_addr + 3, 5);

        vpn[3] = '\0';
        offset[5] = '\0';

        if (running_mode == DEBUG) {
            printf("%s %c\n", vpn, op_type);
            printf("%ld %c\n", strtol(vpn, NULL, 16), op_type);
            //printf("%s %c\n", offset, op_type);
            //printf("%s\n", virtual_addr);
        }

        if (++events_ctr < 3000) { // DELETE ME

            newPTE.int_VA = atoi(virtual_addr);

            PTE *pres = PTE_present(&Q, newPTE);
            if (pres == NULL) {
                ++page_faults_ctr;
                if (!dequeue_full(&Q)) {
                    insert_rear_dequeue(&Q, newPTE);
                } else {
                    switch (replace_with) {
                        case LRU:
                            if (running_mode == DEBUG) {
                                printf("LRU replacement\n");
                                printf("LRU time accessed = %d\n", find_LRU(&Q)->time_accessed);
                                printf("VA %d replacing VA %d\n", newPTE.int_VA, find_LRU(&Q)->int_VA);
                            }
                            replace_PTE(&Q, find_LRU(&Q), newPTE);
                            break;
                        case FIFO:
                            if (running_mode == DEBUG) {
                                printf("FIFO replacement\n");
                                printf("Replacing VA: %d\n", Q.front->int_VA);
                            }
                            remove_front_dequeue(&Q);
                            insert_rear_dequeue(&Q, newPTE);
                            break;
                        case VMS:
                            if (running_mode == DEBUG) {
                                printf("VMS replacement\n");
                            }
                            break;
                    }
                }
            } else {
                if (replace_with == LRU) {
                    pres->time_accessed = get_access_time();
                }
            }
            if (running_mode == DEBUG)
                printf("Page faults: %d\n", page_faults_ctr);
        }
        else {
            break;
        }
    }

    printf("\n");
    printf("total memory frames: %d\n", Q.size);
    printf("events in trace: %d\n", events_ctr);
    printf("total disk reads: %d\n", disk_reads_ctr);
    printf("total disk writes: %d\n", disk_writes_ctr);
    printf("\n");

    print_dequeue(&Q);

    printf("\n\n\n");

//    remove_rear_dequeue(&Q);
//    remove_front_dequeue(&Q);
//    print_dequeue(&Q);

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
        newPTE->time_accessed = get_access_time(); // DELETE ME
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
        newPTE->time_accessed = get_access_time(); // DELETE ME
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

PTE *PTE_present(dequeue *dq, PTE entry) {
    PTE *iter = dq->rear;

    if (dequeue_empty(dq)) {
        return 0;
    }
    else {
        while (iter != NULL) {
            if (iter->int_VA == entry.int_VA) {
                return iter;
            }
            iter = iter->nextPTE;
        }

        return NULL;
    }
}

PTE *replace_PTE(dequeue *dq, PTE *victim, PTE entry) {
    printf("Replacing...\n");

    if (dq->rear == NULL)
        printf("Rear is NULL\n");
    if (dq->rear == NULL)
        printf("Victim is NULL\n");
    if (victim != NULL) {
        PTE *newPTE = (PTE*) malloc(sizeof(PTE));
        newPTE->int_VA = entry.int_VA;
        newPTE->time_accessed = get_access_time();
        // fill in other struct attributes

        newPTE->prevPTE = victim->prevPTE;
        newPTE->nextPTE = victim->nextPTE;

        if (victim == dq->front) {
            if (dq->front->prevPTE != NULL)
                dq->front = dq->front->prevPTE;
            else
                dq->front = NULL;
            printf("Replacing front...\n"); // DELETE ME
        }

        if (victim == dq->rear) {
            if (dq->rear != NULL)
                dq->rear = dq->rear->nextPTE;
            else
                dq->rear = NULL;
            
            printf("Replacing rear...\n"); // DELETE ME
        }

        if (victim->nextPTE != NULL)
            victim->nextPTE->prevPTE = newPTE;
        
        if (victim->prevPTE != NULL)
            victim->prevPTE->nextPTE = newPTE;

        free(victim);

        return newPTE;
    } else {
        insert_rear_dequeue(dq, entry);
        return NULL;
    }
}

int remove_front_dequeue(dequeue *dq) {
    PTE *old_front = dq->front;

    dq->front->prevPTE->nextPTE = NULL;
    dq->front = dq->front->prevPTE;

    free(old_front);
    dq->size--;

    return dq->size;
}

int remove_rear_dequeue(dequeue *dq) {
    PTE *old_rear = dq->rear;

    dq->rear->nextPTE->prevPTE = NULL;
    dq->rear = dq->rear->nextPTE;

    free(old_rear);
    dq->size--;

    return dq->size;
}

void print_dequeue(dequeue *dq) {
    PTE *iter = dq->rear;

    if (dequeue_empty(dq)) {
        printf("The dequeue is empty.\n");
    }
    else {
        while (iter != NULL) {
            printf("This is PTE #%d: %d\n", iter->time_accessed, iter->int_VA);
            iter = iter->nextPTE;
        }
    }
}

PTE *find_LRU(dequeue *dq) {
    if (dequeue_empty(dq)) {
        // Cannot find LRU of empty dequeue
        return NULL;
    }

    PTE *iter = dq->rear;
    PTE *lruPTE = iter;
    if (dq->rear == NULL)
        printf("NULL rear Error!\n"); // DELETE ME

    if (dq->front == NULL)
        printf("NULL front Error!\n"); // DELETE ME

    int min_time_accessed = -1;

    if (iter != NULL)
        min_time_accessed = iter->time_accessed;

    while (iter != NULL) {
        if (iter->time_accessed < min_time_accessed) {
            min_time_accessed = iter->time_accessed;
            lruPTE = iter;
        }

        iter = iter->nextPTE;
    }

    return lruPTE;
}