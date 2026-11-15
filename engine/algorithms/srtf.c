
#include "../include/scheduler.h"
#include "../include/execution_trace.h"
#include <stddef.h>
int schedule_srtf(
    Process processes[],
    int process_count
)
{
    if (processes == NULL || process_count <= 0) {
        return 0;
    }

    for (int i = 0; i < process_count; i++) {
        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            return 0;
        }

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

        /* Select the arrived process with the
           shortest remaining CPU time. */
        for (int i = 0; i < process_count; i++) {

            if (processes[i].remaining_time > 0 &&
                processes[i].arrival_time <= time) {

                if (selected == -1 ||
                    processes[i].remaining_time <
                    processes[selected].remaining_time) {

                    selected = i;
                }
            }
        }

        /* CPU stays idle until the next arrival. */
        if (selected == -1) {
            time++;
            continue;
        }

        Process *p = &processes[selected];

        p->state = PROCESS_RUNNING;

        if (p->first_start_time == -1) {
            p->first_start_time = time;
        }

        /* Execute for one time unit.
           Re-evaluate priorities at the next time unit. */
        p->remaining_time--;
        time++;
        if (!execution_trace_record(p->pid, time - 1, time)) {
            return 0;
        }

        if (p->remaining_time == 0) {

            p->completion_time = time;
            p->state = PROCESS_COMPLETED;

            calculate_process_metrics(p);

            finished++;
        } else {
            p->state = PROCESS_READY;
        }
    }

    return 1;
}
