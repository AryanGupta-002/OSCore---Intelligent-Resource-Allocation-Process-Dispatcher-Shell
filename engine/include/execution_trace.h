#ifndef EXECUTION_TRACE_H
#define EXECUTION_TRACE_H
#include <stddef.h>  // size_t data type use krne ke liye
// Ye structure ek process ke CPU execution ka time record krta h
typedef struct {
    int pid;        // Process ki ID
    int start_time; // Process ne CPU kab start kiya
    int end_time;   // Process ne CPU kab chhoda
} ExecutionTraceSegment;
// Purana execution trace reset krne ke liye function declaration
void execution_trace_reset(void);
// Process ka execution start aur end time trace me record krne ke liye
int execution_trace_record(int pid, int start_time, int end_time);
// Total kitne execution segments record hue h, unka count return krta h
size_t execution_trace_count(void);
// Given index par stored execution segment ka address return krta h
// Agar index valid nhi hua toh NULL return ho skta h
const ExecutionTraceSegment *execution_trace_get(size_t index);
#endif
