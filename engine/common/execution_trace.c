#include <stdlib.h>
#include "execution_trace.h"
// Ye variables saare execution records ko manage karenge
static ExecutionTraceSegment *segments;
static size_t segment_count;
static size_t segment_capacity;
// Purane trace ko reset krne ke liye count zero kr do
// Memory free nhi hogi, reuse ho sakti h
void execution_trace_reset(void)
{ segment_count = 0;
}
int execution_trace_record(int pid, int start_time, int end_time)
{ // PID aur execution timing valid honi chahiye
    if (pid <= 0 || start_time < 0 || end_time <= start_time) {
        return 0;
    }
    // Last segment check kro... same process continue ho raha ho sakta h
    if (segment_count > 0) {
        ExecutionTraceSegment *last = &segments[segment_count - 1];
        // Same process h aur previous segment yahi end hua tha
        // to dono segments ko merge kr sakte hain
        if (last->pid == pid && last->end_time == start_time) {
            // Naya segment banane ki jagah end time update kr do
            last->end_time = end_time;
            return 1;
        }
    }
    // Array full ho gaya h to extra memory allocate krni padegi
    if (segment_count == segment_capacity) {
        // Capacity double kr do... first time 32 se start hogi
        size_t new_capacity = segment_capacity ? segment_capacity * 2 : 32;
        ExecutionTraceSegment *new_segments = realloc(
            segments,
            new_capacity * sizeof(*segments)
        );
        // Memory allocate nhi hui to record save nhi kar sakte
        if (new_segments == NULL) {
            return 0;
        }
        // New memory address aur capacity update kr do
        segments = new_segments;
        segment_capacity = new_capacity;
    }
    // New execution segment ko array mai store kr do
    // PID, start time aur end time save honge
    segments[segment_count++] = (ExecutionTraceSegment) {
        .pid = pid,
        .start_time = start_time,
        .end_time = end_time
    };
    return 1;
}

// Abhi tak kitne execution segments store hue hain wo return hoga
size_t execution_trace_count(void)
{
    return segment_count;
}
// Given index ka execution segment return krna h
// Index valid nhi hua to NULL return hoga
const ExecutionTraceSegment *execution_trace_get(size_t index)
{
    return index < segment_count ? &segments[index] : NULL;
}
