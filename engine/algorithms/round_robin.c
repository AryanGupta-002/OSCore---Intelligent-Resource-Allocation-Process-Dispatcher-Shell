
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"

int schedule_round_robin(
    Process processes[],
    int process_count,
    int time_quantum
)
{
    if (processes == NULL ||
        process_count <= 0 ||
        time_quantum <= 0) {
        return 0;
    }

    int *queue = malloc(
        (size_t)(process_count + 1) * sizeof(int)
    );

    if (queue == NULL) {
        return 0;
    }

    /* Reset every process before simulation. */
    for (int i = 0; i < process_count; i++) {

        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            free(queue);
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

    int front = 0;
    int rear = 0;
    int queue_count = 0;

    while (finished < process_count) {

        /* Add arrived processes in input order. */
        for (int i = 0; i < process_count; i++) {
            if (processes[i].state == PROCESS_NEW &&
                processes[i].arrival_time <= time) {

                queue[rear] = i;
                rear = (rear + 1) % (process_count + 1);
                queue_count++;

                processes[i].state = PROCESS_READY;
            }
        }

        /* If the queue is empty, jump to next arrival. */
        if (queue_count == 0) {

            int next = -1;

            for (int i = 0; i < process_count; i++) {
                if (processes[i].state == PROCESS_NEW &&
                    (next == -1 ||
                     processes[i].arrival_time <
                     processes[next].arrival_time)) {
                    next = i;
                }
            }

            if (next == -1) {
                break;
            }

            time = processes[next].arrival_time;
            continue;
        }

        /* Remove the oldest ready process. */
        int index = queue[front];
        front = (front + 1) % (process_count + 1);
        queue_count--;

        Process *p = &processes[index];

        p->state = PROCESS_RUNNING;

        if (p->first_start_time == -1) {
            p->first_start_time = time;
        }

        int run_time = p->remaining_time;

        if (run_time > time_quantum) {
            run_time = time_quantum;
        }

        int start_time = time;
        time += run_time;
        if (!execution_trace_record(p->pid, start_time, time)) {
            free(queue);
            return 0;
        }
        p->remaining_time -= run_time;

        /*
         * Add newly arrived processes before re-queuing
         * the process whose quantum just ended.
         */
        for (int i = 0; i < process_count; i++) {
            if (processes[i].state == PROCESS_NEW &&
                processes[i].arrival_time <= time) {

                queue[rear] = i;
                rear = (rear + 1) % (process_count + 1);
                queue_count++;

                processes[i].state = PROCESS_READY;
            }
        }

        if (p->remaining_time == 0) {

            p->completion_time = time;
            p->state = PROCESS_COMPLETED;

            calculate_process_metrics(p);
            finished++;

        } else {

            p->state = PROCESS_READY;

            /* Put unfinished process at the queue's end. */
            queue[rear] = index;
            rear = (rear + 1) % (process_count + 1);
            queue_count++;

        }
    }

    free(queue);

    return finished == process_count;
}
