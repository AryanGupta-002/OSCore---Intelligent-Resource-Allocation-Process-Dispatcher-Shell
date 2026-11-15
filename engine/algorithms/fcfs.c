
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"

/*
 * First Come First Serve (FCFS)
 *
 * Adapted from the submitted scheduling_algo PDF.
 * Input: shared Process array loaded from CSV.
 * Output: updated Process metrics and states.
 *
 * time_quantum is unused by FCFS.
 */
int schedule_fcfs(
    Process processes[],
    int process_count
)
{

    if (processes == NULL || process_count <= 0) {
        return 0;
    }

    /* Create an array of indices to preserve input order. */
    int *order = malloc(
        (size_t)process_count * sizeof(int)
    );

    if (order == NULL) {
        return 0;
    }

    for (int i = 0; i < process_count; i++) {

        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            free(order);
            return 0;
        }

        order[i] = i;

        /* Reset results for a fresh algorithm run. */
        processes[i].remaining_time =
            processes[i].burst_time;

        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
        processes[i].first_start_time = -1;

        processes[i].state = PROCESS_NEW;
    }

    /*
     * Sort indices by arrival time.
     * Equal arrival times retain their original order.
     * The Process array itself is not rearranged.
     */
    for (int i = 0; i < process_count - 1; i++) {
        for (int j = i + 1; j < process_count; j++) {

            if (processes[order[i]].arrival_time >
                processes[order[j]].arrival_time) {

                int temp = order[i];
                order[i] = order[j];
                order[j] = temp;
            }
        }
    }

    int time = 0;

    for (int i = 0; i < process_count; i++) {

        Process *p = &processes[order[i]];

        /* Wait until this process has arrived. */
        if (time < p->arrival_time) {
            time = p->arrival_time;
        }

        int start_time = time;
        p->state = PROCESS_RUNNING;
        p->first_start_time = time;

        /* FCFS runs the process until completion. */
        time += p->burst_time;
        if (!execution_trace_record(p->pid, start_time, time)) {
            free(order);
            return 0;
        }

        p->remaining_time = 0;
        p->completion_time = time;
        p->state = PROCESS_COMPLETED;

        /* Calculate CT, TAT, WT and response time. */
        calculate_process_metrics(p);
    }

    free(order);

    return 1;
}
