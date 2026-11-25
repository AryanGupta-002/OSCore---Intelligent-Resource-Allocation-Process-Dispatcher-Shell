//Aryan Wrotee this

#include <stdio.h>
#include <string.h>

#include "host.h"

/* Record an event with the current hardware resource snapshot. */
static int emit_event(
    Host *host,
    const Process *p,
    EventType type,
    int start_time,
    int end_time,
    const char *details
)
{
    ExecutionEvent event = {0};

    event.pid = (p != NULL) ? p->pid : 0;
    event.type = type;
    event.start_time = start_time;
    event.end_time = end_time;

    event.priority = (p != NULL) ? p->priority : -1;
    event.memory_start_mb =
        (p != NULL) ? p->memory_start_mb : -1;
    event.memory_size_mb =
        (p != NULL) ? p->memory_required_mb : 0;

    event.printers_available =
        host->resources.printers_available;
    event.scanners_available =
        host->resources.scanners_available;
    event.modems_available =
        host->resources.modems_available;
    event.cds_available =
        host->resources.cds_available;

    if (details != NULL) {
        snprintf(
            event.details,
            sizeof(event.details),
            "%s",
            details
        );
    }

    return record_event(&host->event_log, &event);
}

/* Explain why a process can never fit within HOST limits. */
static const char *impossible_admission_reason(
    const Host *host,
    const Process *p
)
{
    int memory_limit;

    if (host == NULL || p == NULL) {
        return "HOST or process data is unavailable.";
    }

    if (p->priority < REALTIME_PRIORITY ||
        p->priority > PRIORITY_LEVEL_3) {
        return "Priority must be between 0 and 3.";
    }

    memory_limit = (p->priority == REALTIME_PRIORITY)
        ? host->realtime_memory_capacity_mb
        : host->user_memory_capacity_mb;

    if (p->memory_required_mb <= 0) {
        return "Memory requirement must be positive.";
    }
    if (p->memory_required_mb > memory_limit) {
        return "Memory requirement exceeds the HOST memory pool for this priority.";
    }

    if (p->printers_required < 0 ||
        p->scanners_required < 0 ||
        p->modems_required < 0 ||
        p->cds_required < 0) {
        return "Device requirements cannot be negative.";
    }

    if (p->printers_required > host->config.printers) {
        return "Printer requirement exceeds the configured HOST capacity.";
    }
    if (p->scanners_required > host->config.scanners) {
        return "Scanner requirement exceeds the configured HOST capacity.";
    }
    if (p->modems_required > host->config.modems) {
        return "Modem requirement exceeds the configured HOST capacity.";
    }
    if (p->cds_required > host->config.cds) {
        return "CD requirement exceeds the configured HOST capacity.";
    }
    if (p->burst_time <= 0) {
        return "CPU burst must be positive.";
    }
    if (p->arrival_time < 0) {
        return "Arrival time cannot be negative.";
    }

    return NULL;
}

/* Admit arrived processes whenever memory and devices are available. */
static int admit_arrived_processes(
    Host *host,
    Process processes[],
    int process_count
)
{
    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        const char *rejection_reason;

        if (p->state != PROCESS_NEW ||
            p->arrival_time > host->current_time) {
            continue;
        }

        rejection_reason = impossible_admission_reason(host, p);
        if (rejection_reason != NULL) {
            p->state = PROCESS_REJECTED;
            host->rejected_processes++;

            if (!emit_event(
                    host, p, EVENT_PROCESS_REJECTED,
                    host->current_time, host->current_time,
                    rejection_reason
                )) {
                return 0;
            }

            continue;
        }

        /* Temporary shortages are retried at later simulation times. */
        if (!admit_process(host, p)) {
            continue;
        }
    }

    return 1;
}

/* Check whether another process is ready to use the CPU. */
static int another_process_is_ready(
    Process processes[],
    int process_count,
    int current_index
)
{
    for (int i = 0; i < process_count; i++) {
        if (i != current_index &&
            processes[i].state == PROCESS_READY) {
            return 1;
        }
    }

    return 0;
}

/* Initialize HOST memory, devices, and event history. */
int initialize_host(Host *host, const HostConfig *config)
{
    HostConfig selected = {
        TOTAL_MEMORY_MB,
        TOTAL_PRINTERS,
        TOTAL_SCANNERS,
        TOTAL_MODEMS,
        TOTAL_CDS
    };
    int reserved_memory;

    if (host == NULL) {
        return 0;
    }

    if (config != NULL) selected = *config;
    if (selected.total_memory_mb < 2 ||
        selected.printers < 0 ||
        selected.scanners < 0 ||
        selected.modems < 0 ||
        selected.cds < 0) {
        return 0;
    }

    memset(host, 0, sizeof(*host));
    host->config = selected;

    if (!initialize_event_log(&host->event_log)) {
        return 0;
    }

    reserved_memory = selected.total_memory_mb < RESERVED_MEMORY_MB * 2
        ? selected.total_memory_mb / 2
        : RESERVED_MEMORY_MB;
    host->realtime_memory_capacity_mb = reserved_memory;
    host->user_memory_capacity_mb =
        selected.total_memory_mb - reserved_memory;
    host->realtime_memory =
        create_memory(0, host->realtime_memory_capacity_mb);

    host->user_memory =
        create_memory(
            reserved_memory,
            host->user_memory_capacity_mb
        );

    if (host->realtime_memory == NULL ||
        host->user_memory == NULL) {
        destroy_memory(host->realtime_memory);
        destroy_memory(host->user_memory);
        destroy_event_log(&host->event_log);

        host->realtime_memory = NULL;
        host->user_memory = NULL;

        return 0;
    }

    initialize_resources(
        &host->resources,
        selected.printers,
        selected.scanners,
        selected.modems,
        selected.cds
    );

    return 1;
}

/* Release HOST-owned data structures. */
void destroy_host(Host *host)
{
    if (host == NULL) {
        return;
    }

    destroy_memory(host->realtime_memory);
    destroy_memory(host->user_memory);
    destroy_event_log(&host->event_log);

    host->realtime_memory = NULL;
    host->user_memory = NULL;
}

/* Admit a process and allocate its memory and hardware resources. */
int admit_process(Host *host, Process *p)
{
    MemoryBlock *pool;
    MemoryBlock *block;

    if (host == NULL || p == NULL ||
        p->state != PROCESS_NEW) {
        return 0;
    }

    pool = (p->priority == REALTIME_PRIORITY)
        ? host->realtime_memory
        : host->user_memory;

    if (!can_allocate_resources(p, &host->resources)) {
        return 0;
    }

    block = allocate_memory(pool, p->memory_required_mb);

    if (block == NULL) {
        return 0;
    }

    if (!allocate_resources(p, &host->resources)) {
        free_memory(block);
        merge_free_blocks(pool);
        return 0;
    }

    p->memory_block = block;
    p->memory_start_mb = block->start_mb;
    p->resources_allocated = 1;
    p->state = PROCESS_READY;

    if (!emit_event(
            host, p, EVENT_MEMORY_ALLOCATED,
            host->current_time, host->current_time,
            "First Fit memory allocation succeeded."
        ) ||
        !emit_event(
            host, p, EVENT_RESOURCE_ALLOCATED,
            host->current_time, host->current_time,
            "Required hardware resources allocated."
        ) ||
        !emit_event(
            host, p, EVENT_PROCESS_ADMITTED,
            host->current_time, host->current_time,
            "Process admitted and marked READY."
        )) {

        /*
         * Roll back the admission if event logging fails.
         * Release actual resources even if an event cannot be recorded.
         */
        if (p->memory_block != NULL) {
            free_memory(p->memory_block);
            merge_free_blocks(pool);
            p->memory_block = NULL;
            p->memory_start_mb = -1;
        }

        if (p->resources_allocated) {
            release_resources(p, &host->resources);
            p->resources_allocated = 0;
        }

        p->state = PROCESS_NEW;
        return 0;
    }

    return 1;
}

/* Release allocated memory and devices. */
void release_process_resources(Host *host, Process *p)
{
    MemoryBlock *pool;

    if (host == NULL || p == NULL) {
        return;
    }

    if (p->memory_block != NULL) {
        pool = (p->priority == REALTIME_PRIORITY)
            ? host->realtime_memory
            : host->user_memory;

        /*
         * Always release actual memory, even if event logging fails.
         */
        emit_event(
            host, p, EVENT_MEMORY_RELEASED,
            host->current_time, host->current_time,
            "Process memory released."
        );

        free_memory(p->memory_block);
        merge_free_blocks(pool);

        p->memory_block = NULL;
        p->memory_start_mb = -1;
    }

    if (p->resources_allocated) {
        emit_event(
            host, p, EVENT_RESOURCE_RELEASED,
            host->current_time, host->current_time,
            "Hardware resources released."
        );

        release_resources(p, &host->resources);
        p->resources_allocated = 0;
    }
}

/* Run the HOST four-level priority dispatcher. */
int run_host(
    Host *host,
    Process processes[],
    int process_count
)
{
    int previous_pid = -1;
    int previous_was_user = 0;

    if (host == NULL || processes == NULL ||
        process_count <= 0 ||
        process_count > MAX_PROCESSES ||
        host->realtime_memory == NULL ||
        host->user_memory == NULL ||
        host->event_log.events == NULL) {
        return 0;
    }

    /* Reset process state for a fresh simulation. */
    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];

        p->remaining_time = p->burst_time;
        p->completion_time = 0;
        p->turnaround_time = 0;
        p->waiting_time = 0;
        p->response_time = 0;
        p->first_start_time = -1;

        p->memory_block = NULL;
        p->memory_start_mb = -1;
        p->resources_allocated = 0;

        p->queue_level = p->priority;
        p->state = PROCESS_NEW;
    }

    host->current_time = 0;
    host->completed_processes = 0;
    host->rejected_processes = 0;
    host->event_log.count = 0;

    while (host->completed_processes +
           host->rejected_processes < process_count) {

        if (!admit_arrived_processes(
                host, processes, process_count)) {
            return 0;
        }

        int index = select_next_process(
            processes, process_count
        );

        if (index == -1) {
            int idle_start = host->current_time;

            host->current_time++;

            if (!emit_event(
                    host, NULL, EVENT_CPU_IDLE,
                    idle_start, host->current_time,
                    "No admitted process was ready to execute."
                )) {
                return 0;
            }

            previous_pid = -1;
            previous_was_user = 0;
            continue;
        }

        Process *p = &processes[index];
        int tick_start = host->current_time;

        /*
         * Record preemption when a real-time process takes the CPU
         * immediately after a user process.
         */
        if (p->priority == REALTIME_PRIORITY &&
            previous_was_user &&
            previous_pid != p->pid) {

            if (!emit_event(
                    host, p, EVENT_PROCESS_PREEMPTED,
                    host->current_time, host->current_time,
                    "Real-time process preempted a user process."
                )) {
                return 0;
            }
        }

        p->state = PROCESS_RUNNING;

        if (p->first_start_time == -1) {
            p->first_start_time = host->current_time;
        }

        p->remaining_time--;
        host->current_time++;

        if (!emit_event(
                host, p, EVENT_CPU_EXECUTION,
                tick_start, host->current_time,
                "Process executed for one CPU time unit."
            )) {
            return 0;
        }

        /* Remember who just used the CPU. */
        previous_pid = p->pid;
        previous_was_user = (p->priority != REALTIME_PRIORITY);

        if (p->remaining_time == 0) {
            p->completion_time = host->current_time;
            p->state = PROCESS_COMPLETED;

            calculate_process_metrics(p);

            if (!emit_event(
                    host, p, EVENT_PROCESS_COMPLETED,
                    host->current_time, host->current_time,
                    "Process completed its CPU burst."
                )) {
                return 0;
            }

            release_process_resources(host, p);

            if (p->memory_block != NULL ||
                p->resources_allocated) {
                return 0;
            }

            host->completed_processes++;
        } else {
            p->state = PROCESS_READY;

            if (!admit_arrived_processes(
                    host, processes, process_count)) {
                return 0;
            }

            if (p->priority != REALTIME_PRIORITY &&
                another_process_is_ready(
                    processes, process_count, index)) {

                int old_priority = p->priority;

                demote_user_process(p);

                if (p->priority != old_priority &&
                    !emit_event(
                        host, p, EVENT_PRIORITY_DEMOTED,
                        host->current_time, host->current_time,
                        "User process demoted because another process was ready."
                    )) {
                    return 0;
                }
            }
        }
    }

    return 1;
}
