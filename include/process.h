#ifndef PROCESS_H
#define PROCESS_H

typedef enum {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;


typedef struct {
    int pid;
    char job[15];

    int priority;
    int arrival_time;
    int burst_time;
    int remaining_time;

    int memory_required;

    int printers_required;
    int scanners_required;
    int modems_required;
    int ports_required;

    ProcessState state;

} Process;


void print_process(Process p);

#endif












