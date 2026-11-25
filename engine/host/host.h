// ARYANNNNNNN WAS HERE

#ifndef HOST_H
#define HOST_H

#include "../include/process.h"
#include "../include/event.h"

#include "memory.h"
#include "resource_manager.h"

/* Default HOST configuration */
#define TOTAL_MEMORY_MB 1024
#define RESERVED_MEMORY_MB 64
#define TOTAL_PRINTERS 2
#define TOTAL_SCANNERS 1
#define TOTAL_MODEMS 1
#define TOTAL_CDS 2

/* HOST priority levels */
#define REALTIME_PRIORITY 0
#define PRIORITY_LEVEL_1 1
#define PRIORITY_LEVEL_2 2
#define PRIORITY_LEVEL_3 3

#define MAX_PROCESSES 1000

typedef struct {
    int total_memory_mb;
    int printers;
    int scanners;
    int modems;
    int cds;
} HostConfig;

typedef struct {
    MemoryBlock *realtime_memory;
    MemoryBlock *user_memory;

    HostConfig config;
    int realtime_memory_capacity_mb;
    int user_memory_capacity_mb;
    ResourceManager resources;
    EventLog event_log;

    int current_time;
    int completed_processes;
    int rejected_processes;
} Host;

/* Initialize HOST with the supplied capacities, or defaults when NULL. */
int initialize_host(Host *host, const HostConfig *config);

/* Execute the HOST simulation. */
int run_host(
    Host *host,
    Process processes[],
    int process_count
);

/* Clean up HOST memory and event logging. */
void destroy_host(Host *host);

/* Dispatcher operations. */
int select_next_process(
    Process processes[],
    int process_count
);

void demote_user_process(Process *p);

/* Process admission and resource release. */
int admit_process(Host *host, Process *p);

void release_process_resources(
    Host *host,
    Process *p
);

#endif
