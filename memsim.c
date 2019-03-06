#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <limits.h>

#define PAGE_SIZE 4096

int queue_size = 0;
int queue_capacity = 0;
int global_time_accessed = 0;
int disk_writes_ctr = 0;
int disk_reads_ctr = 0;
int events_ctr = 0;
int hits_ctr = 0;
int fault_ctr = 0;


int p1_list_size = 0;
int p2_list_size = 0;
int RSS_1 = 0;
int RSS_2 = 0;
int clean_list_size = 0;
int dirty_list_size = 0;
int clean_list_capacity = 0;
int dirty_list_capacity = 0;

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
    int PID;

    TAILQ_ENTRY(PTE) page_table;
} PTE;
typedef TAILQ_HEAD(tailhead, PTE) head_t;

enum algo_types{LRU, FIFO, VMS};
enum mode_types{QUIET, DEBUG};
enum mode_types running_mode;

void lru(head_t head, struct PTE *newPTE);
void fifo(head_t head, struct PTE *newPTE);
void vms(head_t *head1, head_t *head2, head_t *cleanhead, head_t *dirtyhead, PTE *newPTE);
void replace_PTE(head_t *head, PTE *victim, PTE *entry);
void print_dequeue(head_t *head);
PTE *PTE_present(head_t *head, PTE *entry); // maybe return pointer to entry for use in replace PTE
PTE *find_LRU(head_t *head);
int dequeue_full();
void PTE_reclaim(head_t *global, head_t *process, PTE *entry);

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

    queue_capacity = nframes;
    RSS_1 = nframes/2;
    RSS_2 = nframes/2;
    clean_list_capacity = nframes/2+1;
    dirty_list_capacity = nframes/2+1;

    enum algo_types replace_with;

    if (strcmp(algo, "lru") == 0) {
        replace_with = LRU;
    } else if (strcmp(algo, "fifo") == 0) {
        replace_with = FIFO;
    }  else if (strcmp(algo, "vms") == 0) {
        replace_with = VMS;
        // ignore user's specification for trace on VMS
        input_file = "gcc.trace";
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

    if (mode == NULL)  {
        printf("No mode given\n");
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
    head_t head;
    head_t head1;
    head_t head2;
    head_t head_dirty;
    head_t head_clean;

    TAILQ_INIT(&head);
    TAILQ_INIT(&head1);
    TAILQ_INIT(&head2);
    TAILQ_INIT(&head_dirty);
    TAILQ_INIT(&head_clean);

    while(fscanf(trace_file, "%x %c", &virtual_addr, &op_type) != EOF){
        ++events_ctr;

        PTE *newPTE = malloc(sizeof(PTE));

        if (op_type == 'W') // TODO: This should occur when the victim page had a write operation and was dirty
            newPTE->dirty = 1;

        newPTE->virtual_page_number = virtual_addr / PAGE_SIZE;
        newPTE->time_accessed = get_access_time();


        if (running_mode == DEBUG) {
            printf("Reading VA: %x %c\n", virtual_addr, op_type);
            printf("Reading VPN: %x\n", newPTE->virtual_page_number);
        }

        if (!dequeue_full() && replace_with != VMS) {
            TAILQ_INSERT_TAIL(&head, newPTE, page_table);
            ++queue_size;
            continue;
        }

        if (replace_with == VMS) {
            vms(&head1, &head2, &head_clean, &head_dirty, newPTE);
        } else if (replace_with == LRU) {
            lru(head, newPTE);
        } else if (replace_with == FIFO) {
            fifo(head, newPTE);
        }

        PTE *pres = PTE_present(&head, newPTE);

        if (pres == NULL) {
            ++fault_ctr;
            if (running_mode == DEBUG)
                printf("faults: %d\n", fault_ctr);

            if (op_type == 'R')
                ++disk_reads_ctr;


        } else {
            ++hits_ctr;

            if (op_type == 'W')
                pres->dirty = 1;

            if (replace_with == LRU) {
                pres->time_accessed = get_access_time();
            }
        }
    }

    printf("total memory frames: %d\n", queue_capacity);
    printf("events in trace: %d\n", events_ctr);
    printf("total disk reads: %d\n", disk_reads_ctr);
    printf("total disk writes: %d\n", disk_writes_ctr);
    printf("\n");

    if(running_mode == DEBUG) print_dequeue(&head);

    // Closes trace file
    fclose(trace_file);
    return 0;
}

void lru(head_t head, struct PTE *newPTE){
    if (running_mode == DEBUG) {
        printf("LRU replacement\n");
        printf("LRU time accessed = %d\n", find_LRU(&head)->time_accessed);
        printf("LRU dirty bit = %d\n", find_LRU(&head)->dirty);
        printf("VA %x replacing VA %x\n", newPTE->virtual_page_number, find_LRU(&head)->virtual_page_number);
    }

    PTE *pres = PTE_present(&head, newPTE);

    if (pres == NULL) {
        ++fault_ctr;
        ++disk_reads_ctr;

        struct PTE *lruPTE = find_LRU(&head);

        if (lruPTE->dirty == 1)
            ++disk_writes_ctr;
        replace_PTE(&head, lruPTE, newPTE);
    } else {
        ++hits_ctr;

        if (newPTE->dirty == 1)
            pres->dirty = 1;

        pres->time_accessed = get_access_time();
    }
}

void fifo(head_t head, struct PTE *newPTE){
    if (running_mode == DEBUG) {
        printf("FIFO replacement\n");
        printf("Replacing VA: %x\n", TAILQ_FIRST(&head)->virtual_page_number);
    }
    
    PTE *pres = PTE_present(&head, newPTE);

    if (pres == NULL) {
        if (dequeue_full()) {
            struct PTE *first = TAILQ_FIRST(&head);

            if (first->dirty == 1)
                ++disk_writes_ctr;

            TAILQ_REMOVE(&head, first, page_table);
            queue_size--;
        }
        TAILQ_INSERT_TAIL(&head, newPTE, page_table);
        ++queue_size;
    } else {
        ++hits_ctr;

        if (newPTE->dirty == 1)
            pres->dirty = 1;
    }
}

void vms(head_t *head1, head_t *head2, head_t *cleanhead, head_t *dirtyhead, PTE *newPTE) {
        unsigned first_digit = newPTE->virtual_page_number / 65536;

        if (first_digit == 3) {
            newPTE->PID = 1;
            if (running_mode == DEBUG) printf("New page belongs to P1: %d\n", first_digit);
        } else {
            newPTE->PID = 2;
            if (running_mode == DEBUG) printf("New page belongs to P2 %d\n", first_digit);
        }

        PTE *foundPTE = NULL;
        if (newPTE->PID == 1) {
            if ( (foundPTE = PTE_present(head1, newPTE)) != NULL) {
                ++hits_ctr;
            } else if ( (foundPTE = PTE_present(cleanhead, newPTE)) != NULL) {
                TAILQ_REMOVE(cleanhead, foundPTE, page_table);

                if (!TAILQ_EMPTY(head1)) {
                    struct PTE *first_p1 = TAILQ_FIRST(head1);
                    TAILQ_REMOVE(head1, first_p1, page_table);
//                free(first_p1);
                }

                TAILQ_INSERT_TAIL(head1, foundPTE, page_table);
            } else if ( (foundPTE = PTE_present(dirtyhead, newPTE)) != NULL) {
                TAILQ_REMOVE(dirtyhead, foundPTE, page_table);

                if (!TAILQ_EMPTY(head1)) {
                    struct PTE *first_p1 = TAILQ_FIRST(head1);
                    TAILQ_REMOVE(head1, first_p1, page_table);
                }

                TAILQ_INSERT_TAIL(head1, foundPTE, page_table);
            } else {
                ++fault_ctr;

                if (p1_list_size == RSS_1) {
                    if (running_mode == DEBUG) printf("P1 list is full\n");

                    struct PTE *first_p1 = TAILQ_FIRST(head1);
                    TAILQ_REMOVE(head1, first_p1, page_table);
                    p1_list_size--;

                    if (first_p1->dirty == 1) {
                        if (dirty_list_size == dirty_list_capacity) {
                            ++disk_writes_ctr;

                            struct PTE *first_dirty = TAILQ_FIRST(dirtyhead);

                            TAILQ_REMOVE(dirtyhead, first_dirty, page_table);
                            TAILQ_INSERT_TAIL(dirtyhead, first_p1, page_table);

//                            free(first_dirty);
                        } else {
                            TAILQ_INSERT_TAIL(dirtyhead, first_p1, page_table);
                            ++dirty_list_size;
                        }
                    } else {
                        if (clean_list_size == clean_list_capacity) {

                            struct PTE *first_clean = TAILQ_FIRST(cleanhead);

                            TAILQ_REMOVE(cleanhead, first_clean, page_table);
                            TAILQ_INSERT_TAIL(cleanhead, first_p1, page_table);

//                            free(first_clean);
                        } else {
                            TAILQ_INSERT_TAIL(cleanhead, first_p1, page_table);
                            ++clean_list_size;
                        }
                    }
                } 
                TAILQ_INSERT_TAIL(head1, newPTE, page_table);
                ++p1_list_size;
            }
        } else if (newPTE->PID == 2) {
            if ( (foundPTE = PTE_present(head1, newPTE)) != NULL) {
                ++hits_ctr;

            } else if ( (foundPTE = PTE_present(cleanhead, newPTE)) != NULL) {
                TAILQ_REMOVE(cleanhead, foundPTE, page_table);

                if (!TAILQ_EMPTY(head2)) {
                struct PTE *first_p2 = TAILQ_FIRST(head2);
                TAILQ_REMOVE(head2, first_p2, page_table);
//                free(first_p1);
                }

                TAILQ_INSERT_TAIL(head2, foundPTE, page_table);

            } else if ( (foundPTE = PTE_present(dirtyhead, newPTE)) != NULL) {
                TAILQ_REMOVE(dirtyhead, foundPTE, page_table);

                if (!TAILQ_EMPTY(head2)) {
                    struct PTE *first_p2 = TAILQ_FIRST(head2);
                    TAILQ_REMOVE(head2, first_p2, page_table);
                }

                TAILQ_INSERT_TAIL(head2, foundPTE, page_table);
            } else {
                ++fault_ctr;

                if (p2_list_size == RSS_2) {
                    if (running_mode == DEBUG) printf("P2 list is full\n");
                    
                    struct PTE *first_p2 = TAILQ_FIRST(head2);
                    TAILQ_REMOVE(head2, first_p2, page_table);
                    p2_list_size--;

                    if (first_p2->dirty == 1) {
                        if (dirty_list_size == dirty_list_capacity) {
                            ++disk_writes_ctr;

                            struct PTE *first_dirty = TAILQ_FIRST(dirtyhead);

                            TAILQ_REMOVE(dirtyhead, first_dirty, page_table);
                            TAILQ_INSERT_TAIL(dirtyhead, first_p2, page_table);

//                            free(first_dirty);
                        } else {
                            TAILQ_INSERT_TAIL(dirtyhead, first_p2, page_table);
                            ++dirty_list_size;
                        }
                    } else {
                        if (clean_list_size == clean_list_capacity) {

                            struct PTE *first_clean = TAILQ_FIRST(cleanhead);

                            TAILQ_REMOVE(cleanhead, first_clean, page_table);
                            TAILQ_INSERT_TAIL(cleanhead, first_p2, page_table);

//                            free(first_clean);
                        } else {
                            TAILQ_INSERT_TAIL(cleanhead, first_p2, page_table);
                            ++clean_list_size;
                        }
                    }
                }
                TAILQ_INSERT_TAIL(head2, newPTE, page_table);
                ++p2_list_size;
            }
        }
}

void PTE_reclaim(head_t *global, head_t *process, PTE *entry){
    struct PTE *reclaimer = PTE_present(global, entry);
    if(reclaimer != NULL){
        TAILQ_INSERT_TAIL(process, entry, page_table);
    }
    else{
        TAILQ_INSERT_TAIL(global, entry, page_table);
    }
}

PTE *PTE_present(head_t *head, PTE *entry) {
    if(running_mode == DEBUG) printf("Inside PTE present\n");

    struct PTE *page;
    TAILQ_FOREACH(page, head, page_table){
        if(page->virtual_page_number == entry->virtual_page_number)
            return page;
    }
    return NULL;
}

void replace_PTE(head_t *head, PTE *victim, PTE *entry) {
    if(running_mode == DEBUG) printf("Replacing...\n");

    entry = malloc(sizeof(PTE));

    TAILQ_INSERT_AFTER(head, victim, entry, page_table);
    TAILQ_REMOVE(head, victim, page_table);
}

void print_dequeue(head_t *head) {
    if (TAILQ_EMPTY(head)) {
        printf("The dequeue is empty.\n");
    }

    struct PTE *page;
    TAILQ_FOREACH(page, head, page_table){
        printf("PTE: %d, VA: %x\n", page->time_accessed, page->virtual_page_number);
    }
}

PTE *find_LRU(head_t *head) {
    if(running_mode == DEBUG) printf("Entered find LRU\n");

    if (TAILQ_EMPTY(head)){
        // Cannot find LRU of empty dequeue
        if(running_mode == DEBUG) printf("Queue is empty in find_LRU\n");
        return NULL;
    }

    struct PTE *lruPTE;
    struct PTE *page;
    int min = INT_MAX;
    TAILQ_FOREACH(page, head, page_table){
        if(page->time_accessed < min){
            lruPTE = page;
            min = page->time_accessed;
        }
    }
    return lruPTE;
}

int dequeue_full(){
    return queue_size == queue_capacity;
}