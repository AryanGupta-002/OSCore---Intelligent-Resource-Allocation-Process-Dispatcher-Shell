#include <stdlib.h>

#include "execution_trace.h"

static ExecutionTraceSegment *segments;
static size_t segment_count;
static size_t segment_capacity;

void execution_trace_reset(void)
{
    segment_count = 0;
}

int execution_trace_record(int pid, int start_time, int end_time)
{
    if (pid <= 0 || start_time < 0 || end_time <= start_time) {
        return 0;
    }

    if (segment_count > 0) {
        ExecutionTraceSegment *last = &segments[segment_count - 1];
        if (last->pid == pid && last->end_time == start_time) {
            last->end_time = end_time;
            return 1;
        }
    }

    if (segment_count == segment_capacity) {
        size_t new_capacity = segment_capacity ? segment_capacity * 2 : 32;
        ExecutionTraceSegment *new_segments = realloc(
            segments,
            new_capacity * sizeof(*segments)
        );
        if (new_segments == NULL) {
            return 0;
        }
        segments = new_segments;
        segment_capacity = new_capacity;
    }

    segments[segment_count++] = (ExecutionTraceSegment) {
        .pid = pid,
        .start_time = start_time,
        .end_time = end_time
    };
    return 1;
}

size_t execution_trace_count(void)
{
    return segment_count;
}

const ExecutionTraceSegment *execution_trace_get(size_t index)
{
    return index < segment_count ? &segments[index] : NULL;
}
