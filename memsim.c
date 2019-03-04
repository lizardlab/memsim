#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096

int global_time_accessed = 0; // DELETE ME testing

int get_access_time() {
    return ++global_time_accessed;
}

typedef struct PTE {
    unsigned virtual_page_number;
    int offset;

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
        // ignore user's specification for trace on VMS
        input_file = "bzip.trace";
    } else {
        printf("Algorithm not recognized.\n");
        return -1;
    }
    // Opens trace file in read mode
    FILE *trace_file = fopen(input_file, "r");

    if (trace_file == NULL) {
        printf("Failed to open input file.");
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
    unsigned virtual_addr;
    char op_type;
    dequeue Q;
    init_dequeue(&Q, nframes);

    int events_ctr = 0;
    int disk_reads_ctr = 0;
    int disk_writes_ctr = 0;
    int hits_ctr = 0;
    int fault_ctr = 0;
    PTE newPTE;

    while( fscanf(trace_file, "%x %c", &virtual_addr, &op_type) != EOF){
        if (op_type == 'W') // TODO: This should occur when the victim page had a write operation and was dirty
            newPTE.dirty = 1;

        newPTE.virtual_page_number = virtual_addr / PAGE_SIZE;
        if (running_mode == DEBUG) {
            printf("Reading VA: %x %c\n", virtual_addr, op_type);
            printf("Reading VPN: %x\n", newPTE.virtual_page_number);
        }

        if (++events_ctr < 30) { // DELETE ME


            newPTE.virtual_page_number = virtual_addr;
            PTE *pres = PTE_present(&Q, newPTE);
            if (pres == NULL) {
                ++fault_ctr;
                if (running_mode == DEBUG)
                    printf("faults: %d\n", fault_ctr);
                if (op_type == 'R') // TODO: This should occur in case of page fault
                    ++disk_reads_ctr;

                if (!dequeue_full(&Q)) {
                    insert_rear_dequeue(&Q, newPTE);
                } else {
                    switch (replace_with) {
                        case LRU:
                            if (running_mode == DEBUG) {
                                printf("LRU replacement\n");
                                printf("LRU time accessed = %d\n", find_LRU(&Q)->time_accessed);
                                printf("LRU dirty bit = %d\n", find_LRU(&Q)->dirty);
                                printf("VA %x replacing VA %x\n", newPTE.virtual_page_number, find_LRU(&Q)->virtual_page_number);
                            }

                            if (find_LRU(&Q)->dirty == 1)
                                ++disk_writes_ctr;
                            replace_PTE(&Q, find_LRU(&Q), newPTE);
                            break;
                        case FIFO:
                            if (running_mode == DEBUG) {
                                printf("FIFO replacement\n");
                                printf("Replacing VA: %x\n", Q.front->virtual_page_number);
                            }
                            if (Q.front != NULL) {
                                if (op_type == 'W') { // TODO: This should occur when the victim page had a write operation and was dirty
                                    ++disk_writes_ctr;
                                }
                                remove_front_dequeue(&Q);
                            }
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
                ++hits_ctr;
                if (op_type == 'W')
                    pres->dirty = 1;
                if (replace_with == LRU) {
                    pres->time_accessed = get_access_time();
                }
            }
            if (running_mode == DEBUG)
                printf("hits: %d\n", hits_ctr);
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
        newPTE->virtual_page_number = entry.virtual_page_number;

        if (dq->front == NULL) {
            dq->front = newPTE;
            dq->rear = newPTE;
        }
        else {
            newPTE->prevPTE = dq->front;
            dq->front->nextPTE = newPTE;
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
        newPTE->virtual_page_number = entry.virtual_page_number;

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
    printf("Inside PTE present\n");
    PTE *iter = dq->rear;

    if (dequeue_empty(dq)) {
        printf("Inside PTE present and queue is empty\n");
        return NULL;
    }
    else {
        printf("Inside PTE present and queue is NOT empty\n");

        while (iter != NULL) {
            printf("iter is not NULL in PTE Present\n");
            printf("Searching for entry VA: %x \n", entry.virtual_page_number);
            printf("VA = %x\n", iter->virtual_page_number);
            if (iter->virtual_page_number == entry.virtual_page_number) {
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
        newPTE->virtual_page_number = entry.virtual_page_number;
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
            if (dq->rear->nextPTE != NULL)
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

    if (dq->front != NULL) {
        if (dq->front->prevPTE != NULL) {
            dq->front->prevPTE->nextPTE = NULL;
            dq->front = dq->front->prevPTE;
        }
        free(old_front);
    }

    dq->size--;
    return dq->size;
}

int remove_rear_dequeue(dequeue *dq) {
    PTE *old_rear = dq->rear;

    if (dq->rear != NULL) {
        if (dq->rear->nextPTE != NULL) {
            dq->rear->nextPTE->prevPTE = NULL;
            dq->rear = dq->rear->nextPTE;
        }
        free(old_rear);
    }

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
            printf("This is PTE #%d, VA: %x\n", iter->time_accessed, iter->virtual_page_number);
            iter = iter->nextPTE;
        }
    }
}

PTE *find_LRU(dequeue *dq) {
    printf("Entered find LRU\n");
    if (dequeue_empty(dq)) {
        // Cannot find LRU of empty dequeue
        printf("Queue is empty in find_LRU\n");
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