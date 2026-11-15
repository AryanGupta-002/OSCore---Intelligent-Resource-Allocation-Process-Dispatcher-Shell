#ifndef EXECUTION_TRACE_H
#define EXECUTION_TRACE_H

#include <stddef.h>

typedef struct {
    int pid;
    int start_time;
    int end_time;
} ExecutionTraceSegment;

void execution_trace_reset(void);
int execution_trace_record(int pid, int start_time, int end_time);
size_t execution_trace_count(void);
const ExecutionTraceSegment *execution_trace_get(size_t index);

#endif
