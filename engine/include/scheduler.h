
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

typedef enum {
    ALGORITHM_FCFS,
    ALGORITHM_SJF,
    ALGORITHM_PRIORITY,
    ALGORITHM_ROUND_ROBIN,
    ALGORITHM_SRTF,
    ALGORITHM_HOST
} AlgorithmType;

/* Scheduling algorithm declarations. */
int schedule_fcfs(
    Process processes[],
    int process_count
);

int schedule_sjf(
    Process processes[],
    int process_count
);

int schedule_priority(
    Process processes[],
    int process_count
);

int schedule_round_robin(
    Process processes[],
    int process_count,
    int time_quantum
);

int schedule_srtf(
    Process processes[],
    int process_count
);

int schedule_mlfq(
    Process processes[], 
    int process_count
);
#endif
