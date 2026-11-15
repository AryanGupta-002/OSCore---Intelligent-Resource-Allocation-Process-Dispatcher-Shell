
#include <string.h>
#include "../include/process.h"

void initialize_process(Process *p)
{
    p->pid = 0;
    strcpy(p->label, "");

    /* Scheduling information */
    p->arrival_time = 0;
    p->priority = 0;
    p->burst_time = 0;
    p->remaining_time = 0;

    /* Performance metrics */
    p->completion_time = 0;
    p->turnaround_time = 0;
    p->waiting_time = 0;
    p->response_time = 0;
    p->first_start_time = -1;

    /* Resource requirements */
    p->memory_required_mb = 0;
    p->printers_required = 0;
    p->scanners_required = 0;
    p->modems_required = 0;
    p->cds_required = 0;

    /* HOST allocation tracking */
    p->queue_level = 0;
    p->memory_start_mb = -1;
    p->memory_block = NULL;
    p->resources_allocated = 0;

    /* Initial lifecycle state */
    p->state = PROCESS_NEW;
}
