#ifndef EVENT_H
#define EVENT_H
#define EVENT_DETAIL_LENGTH 128  // Event ki extra details store karne ki maximum length
// Ye enum system me hone wale different events ko represent krta h
typedef enum {
    EVENT_CPU_EXECUTION,       // CPU kisi process ko execute kr rha h
    EVENT_PROCESS_ADMITTED,    // Process ko system me allow kr liya gya h
    EVENT_PROCESS_REJECTED,    // Process ko system ne reject kr diya h
    EVENT_PROCESS_PREEMPTED,   // Running process ko CPU se hata diya gya h
    EVENT_PRIORITY_DEMOTED,    // Process ki priority kam kr di gyi h
    EVENT_MEMORY_ALLOCATED,    // Process ko memory assign hui h
    EVENT_MEMORY_RELEASED,     // Process ki memory free hui h
    EVENT_RESOURCE_ALLOCATED,  // Process ko resource assign hua h
    EVENT_RESOURCE_RELEASED,   // Process ka resource release hua h
    EVENT_PROCESS_COMPLETED,   // Process ka execution complete ho gya h
    EVENT_CPU_IDLE             // CPU currently koi process execute nhi kr rha h
} EventType;
// Ye structure ek single event ki complete information store krta h
typedef struct {
    int event_id;  // Har event ki unique ID
    int pid;        // Event kis process se related h
    EventType type; // Event ka type
    int start_time; // Event kab start hua
    int end_time;   // Event kab end hua
    int priority;   // Process ki current priority
    int memory_start_mb; // Memory ka starting address (MB me)
    int memory_size_mb;  // Process ko kitni memory mili (MB me)
    // Available resources ki current quantity store hoti h
    int printers_available;
    int scanners_available;
    int modems_available;
    int cds_available;
    char details[EVENT_DETAIL_LENGTH]; // Event ke baare me extra information
} ExecutionEvent;
// Ye structure saare events ka record maintain krta h
typedef struct {
    ExecutionEvent *events; // Events ko store krne wali dynamic array
    int count;               // Abhi total kitne events store hue h
    int capacity;            // Array me maximum kitne events store ho skte h
} EventLog;
// Event log initialize krne ke liye function declaration
int initialize_event_log(EventLog *log);
// Naya event log me add krne ke liye function declaration
int record_event(
    EventLog *log,
    const ExecutionEvent *event
);
// Event log ki allocated memory free krne ke liye function declaration
void destroy_event_log(EventLog *log);
// Event type ko readable string/name me convert krne ke liye function declaration
const char *event_type_name(EventType type);

#endif
