// code by agrima
#include "../include/process.h"

void calculate_process_metrics(Process *p)
{
    /* Completion time must already be set by the scheduler */

    p->turnaround_time =
        p->completion_time - p->arrival_time;

    p->waiting_time =
        p->turnaround_time - p->burst_time;

    p->response_time =
        p->first_start_time - p->arrival_time;
}
