
#ifndef EVENT_H
#define EVENT_H

#define EVENT_DETAIL_LENGTH 128

typedef enum {
    EVENT_CPU_EXECUTION,
    EVENT_PROCESS_ADMITTED,
    EVENT_PROCESS_REJECTED,
    EVENT_PROCESS_PREEMPTED,
    EVENT_PRIORITY_DEMOTED,
    EVENT_MEMORY_ALLOCATED,
    EVENT_MEMORY_RELEASED,
    EVENT_RESOURCE_ALLOCATED,
    EVENT_RESOURCE_RELEASED,
    EVENT_PROCESS_COMPLETED,
    EVENT_CPU_IDLE
} EventType;

typedef struct {
    int event_id;
    int pid;

    EventType type;

    int start_time;
    int end_time;

    int priority;

    int memory_start_mb;
    int memory_size_mb;

    int printers_available;
    int scanners_available;
    int modems_available;
    int cds_available;

    char details[EVENT_DETAIL_LENGTH];
} ExecutionEvent;

typedef struct {
    ExecutionEvent *events;
    int count;
    int capacity;
} EventLog;

int initialize_event_log(EventLog *log);

int record_event(
    EventLog *log,
    const ExecutionEvent *event
);

void destroy_event_log(EventLog *log);

const char *event_type_name(EventType type);

#endif
