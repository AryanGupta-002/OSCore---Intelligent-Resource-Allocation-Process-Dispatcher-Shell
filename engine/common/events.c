
#include <stdlib.h>
#include <string.h>

#include "../include/event.h"

#define INITIAL_EVENT_CAPACITY 128

int initialize_event_log(EventLog *log)
{
    if (log == NULL) {
        return 0;
    }

    log->events = malloc(
        INITIAL_EVENT_CAPACITY * sizeof(ExecutionEvent)
    );

    if (log->events == NULL) {
        log->count = 0;
        log->capacity = 0;
        return 0;
    }

    log->count = 0;
    log->capacity = INITIAL_EVENT_CAPACITY;

    return 1;
}

int record_event(
    EventLog *log,
    const ExecutionEvent *event
)
{
    if (log == NULL || event == NULL) {
        return 0;
    }

    if (log->count == log->capacity) {
        int new_capacity = log->capacity * 2;

        ExecutionEvent *new_events = realloc(
            log->events,
            (size_t)new_capacity * sizeof(ExecutionEvent)
        );

        if (new_events == NULL) {
            return 0;
        }

        log->events = new_events;
        log->capacity = new_capacity;
    }

    log->events[log->count] = *event;
    log->events[log->count].event_id = log->count + 1;

    log->count++;

    return 1;
}

void destroy_event_log(EventLog *log)
{
    if (log == NULL) {
        return;
    }

    free(log->events);

    log->events = NULL;
    log->count = 0;
    log->capacity = 0;
}

const char *event_type_name(EventType type)
{
    switch (type) {
        case EVENT_CPU_EXECUTION:
            return "CPU_EXECUTION";

        case EVENT_PROCESS_ADMITTED:
            return "PROCESS_ADMITTED";

        case EVENT_PROCESS_REJECTED:
            return "PROCESS_REJECTED";

        case EVENT_PROCESS_PREEMPTED:
            return "PROCESS_PREEMPTED";

        case EVENT_PRIORITY_DEMOTED:
            return "PRIORITY_DEMOTED";

        case EVENT_MEMORY_ALLOCATED:
            return "MEMORY_ALLOCATED";

        case EVENT_MEMORY_RELEASED:
            return "MEMORY_RELEASED";

        case EVENT_RESOURCE_ALLOCATED:
            return "RESOURCE_ALLOCATED";

        case EVENT_RESOURCE_RELEASED:
            return "RESOURCE_RELEASED";

        case EVENT_PROCESS_COMPLETED:
            return "PROCESS_COMPLETED";

        case EVENT_CPU_IDLE:
            return "CPU_IDLE";

        default:
            return "UNKNOWN";
    }
}
