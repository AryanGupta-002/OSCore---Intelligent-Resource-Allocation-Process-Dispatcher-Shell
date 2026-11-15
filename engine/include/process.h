
#ifndef PROCESS_H
#define PROCESS_H

#define MAX_LABEL_LENGTH 32

/* Current state of a process */
typedef enum {
    PROCESS_NEW,
    PROCESS_WAITING,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_COMPLETED,
    PROCESS_REJECTED
} ProcessState;

/* MemoryBlock is defined in host/memory.h */
struct MemoryBlock;

typedef struct {
    int pid;
    char label[MAX_LABEL_LENGTH];

    /* Scheduling information */
    int arrival_time;
    int priority;
    int burst_time;
    int remaining_time;

    /* Performance metrics */
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    int first_start_time;

    /* Memory and device requirements */
    int memory_required_mb;
    int printers_required;
    int scanners_required;
    int modems_required;
    int cds_required;

    /* HOST resource allocation tracking */
    int queue_level;
    int memory_start_mb;
    struct MemoryBlock *memory_block;
    int resources_allocated;

    /* Process lifecycle */
    ProcessState state;

} Process;

/* Initialize a process with default values */
void initialize_process(Process *p);

/* Read workload from a CSV file */
int load_processes(
    const char *filename,
    Process processes[],
    int max_processes
);

/* Calculate standard scheduling metrics */
void calculate_process_metrics(Process *p);

#endif
