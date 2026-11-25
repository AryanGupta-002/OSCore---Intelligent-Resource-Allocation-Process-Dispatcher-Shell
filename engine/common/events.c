#include <stdlib.h>
#include <string.h>

#include "../include/event.h"
#define INITIAL_EVENT_CAPACITY 128
// Starting mai Event Log initialize krna h
int initialize_event_log(EventLog *log)
{
    // Log valid nhi h to aage nhi badhenge
    if (log == NULL) {
        return 0;
    }
    // Starting mai 128 events store krne ke liye memory allocate kro
    log->events = malloc(
        INITIAL_EVENT_CAPACITY * sizeof(ExecutionEvent)
    );
    // Memory allocate nhi hui to values zero kr do
    if (log->events == NULL) {
        log->count = 0;
        log->capacity = 0;
        return 0;
    }
    // Abhi koi event store nhi hua h
    log->count = 0;
    log->capacity = INITIAL_EVENT_CAPACITY;
    return 1;
}
int record_event(EventLog *log,const ExecutionEvent *event)
{// Log ya event valid nhi h to record nhi karenge
    if (log == NULL || event == NULL) {
        return 0;
    }
    // Check kro ki Event Log full ho gaya h ya nhi
    if (log->count == log->capacity) {

        // Space full h to capacity double kr do
        int new_capacity = log->capacity * 2;

        // Existing memory ko bigger size mai resize kro
        ExecutionEvent *new_events = realloc(
            log->events,
            (size_t)new_capacity * sizeof(ExecutionEvent)
        );
        // Memory resize nhi hui to error return kro
        if (new_events == NULL) {
            return 0;
        }
        // New memory address aur capacity update kr do
        log->events = new_events;
        log->capacity = new_capacity;
    }
    // New event ko array mai store kr do
    log->events[log->count] = *event;
    // Har event ko ek unique ID assign kr do
    log->events[log->count].event_id = log->count + 1;
    // Event successfully add hua h... count increase kr do
    log->count++;
    return 1;
}
// Event Log ka memory release krna h
void destroy_event_log(EventLog *log)
{
    // Log valid nhi h to return kr do
    if (log == NULL) {
        return;
    }

    // Dynamically allocated memory free kr do
    free(log->events);

    // Values reset kr do taaki log clean state mai aa jaye
    log->events = NULL;
    log->count = 0;
    log->capacity = 0;
}
// Event type ke according uska readable name return hoga
const char *event_type_name(EventType type)
{ switch (type) {
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

        // Koi matching event type nhi mila to UNKNOWN return kro
        default:
            return "UNKNOWN";
    }
}
