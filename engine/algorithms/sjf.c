
// CODE BY AGRIMA of SJF

#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"

int schedule_sjf(
    Process processes[],
    int process_count
)
{

    if (processes == NULL || process_count <= 0) {
        return 0;
    }

    // array of indices, cause i wanted to preserve input order
    int *completed = calloc(
        (size_t)process_count, sizeof(int)
    );

    if (completed == NULL) {
        return 0;
    }

    for (int i = 0; i < process_count; i++) {

        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            free(completed);
            return 0;
        }

        // metics wali sari values ko zero kardiya taki koi galat sa result store nah ho
        processes[i].remaining_time =
            processes[i].burst_time;

        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
        processes[i].first_start_time = -1;

        processes[i].state = PROCESS_NEW;
    }

    int time = 0;
    int finished = 0;

    while (finished < process_count) {

        int selected = -1;

        // Find the shortest arrived, unfinished process.
        for (int i = 0; i < process_count; i++) {

            if (!completed[i] &&
                processes[i].arrival_time <= time) {

                if (selected == -1 ||
                    processes[i].burst_time <
                    processes[selected].burst_time) {

                    selected = i;
                }
            }
        }

        // No process has arrived yet: advance the clock.
        if (selected == -1) {

            int next_arrival = -1;

            for (int i = 0; i < process_count; i++) {
                if (!completed[i] &&
                    (next_arrival == -1 ||
                     processes[i].arrival_time <
                     processes[next_arrival].arrival_time)) {

                    next_arrival = i;
                }
            }

            time = processes[next_arrival].arrival_time;
            continue;
        }

        Process *p = &processes[selected];

        int start_time = time;
        p->state = PROCESS_RUNNING;
        p->first_start_time = time;

        // Non-preemptive: run until the process completes. 
        time += p->burst_time;
        if (!execution_trace_record(p->pid, start_time, time)) {
            free(completed);
            return 0;
        }

        p->remaining_time = 0;
        p->completion_time = time;
        p->state = PROCESS_COMPLETED;

        calculate_process_metrics(p);

        completed[selected] = 1;
        finished++;
    }

    free(completed);

    return 1;
}
