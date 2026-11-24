// Code by Bibham Bibhuti
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"



int schedule_round_robin(
    Process *processes[],
    int process_count,
    int time_quantum
)
{
    if (processes == NULL ||
        process_count <= 0 ||
        time_quantum <= 0) {
        return 0;
    }

    // Yahan hum ready queue mein saare processes ke indexes store karenge....
    int *queue = malloc(
        (size_t)(process_count + 1) * sizeof(int)
    );

    if (queue == NULL) {
        return 0;
    }

    // Har process ko fresh schedulinng mile isliye yahan hum uske liye wapis se reset kar rahe hai....
    for (int i = 0; i < process_count; i++) {

        if (processes[i] == NULL ||
            processes[i]->arrival_time < 0 ||
            processes[i]->burst_time <= 0) {
            free(queue);
            return 0;
        }

        processes[i]->remaining_time = processes[i]->burst_time;

        processes[i]->completion_time = 0;
        processes[i]->turnaround_time = 0;
        processes[i]->waiting_time = 0;
        processes[i]->response_time = 0;
        processes[i]->first_start_time = -1;

        processes[i]->state = PROCESS_NEW;
    }

    int time = 0;
    int finished = 0;

    int front = 0;
    int rear = 0;
    int queue_count = 0;

    while (finished < process_count) {

        // Arrived ho chuki processes ko input order mein ready queue mein add karengeee...
        for (int i = 0; i < process_count; i++) {

            if (processes[i]->state == PROCESS_NEW &&
                processes[i]->arrival_time <= time) {

                queue[rear] = i;
                rear = (rear + 1) % (process_count + 1);
                queue_count++;

                processes[i]->state = PROCESS_READY;
            }
        }

        // Agr queue empty hai toh next arrival time tak CPU ko idle rakhenge....
        if (queue_count == 0) {

            int next = -1;

            for (int i = 0; i < process_count; i++) {

                if (processes[i]->state == PROCESS_NEW &&
                    (next == -1 ||
                     processes[i]->arrival_time <
                     processes[next]->arrival_time)) {

                    next = i;
                }
            }

            if (next == -1) {
                break;
            }

            time = processes[next]->arrival_time;
            continue;
        }

        // Hum yahan FIFO order mein queue ke front se process ko select karenge ....
        int index = queue[front];

        front = (front + 1) % (process_count + 1);
        queue_count--;

        Process *p = processes[index];

        p->state = PROCESS_RUNNING;

        // jo bhi first execution hoga uska completion time sirf ek baar store karenge...
        if (p->first_start_time == -1) {
            p->first_start_time = time;
        }

        // Ek turn mein process ko maximum time quantum hi milega....
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

        // Newly arrived processes ko unfinished process se pehle add karengee......
        for (int i = 0; i < process_count; i++) {

            if (processes[i]->state == PROCESS_NEW &&
                processes[i]->arrival_time <= time) {

                queue[rear] = i;
                rear = (rear + 1) % (process_count + 1);
                queue_count++;

                processes[i]->state = PROCESS_READY;
            }
        }

        if (p->remaining_time == 0) {

            p->completion_time = time;
            p->state = PROCESS_COMPLETED;

            calculate_process_metrics(p);
            finished++;

        } else {

            p->state = PROCESS_READY;

            //  Agr process complete nahi hua toh usko next turn ke liye
            // ready queue ke end mein bhejenge......
            queue[rear] = index;
            rear = (rear + 1) % (process_count + 1);
            queue_count++;
        }
    }

    free(queue);

    return finished == process_count;
}
