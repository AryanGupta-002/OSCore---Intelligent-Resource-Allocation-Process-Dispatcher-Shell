
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"

/*
 * Multilevel Feedback Queue (MLFQ)
 *
 * Three feedback queues:
 * Q0 - Highest priority - Time quantum: 2
 * Q1 - Medium priority - Time quantum: 4
 * Q2 - Lowest priority  - Time quantum: 8
 *
 * Rules:
 * 1. Every new process enters Q0.
 * 2. Each queue uses Round Robin scheduling.
 * 3. An unfinished process is demoted after using
 *    its complete time quantum.
 * 4. A process in a higher-priority queue can
 *    preempt a process running in a lower queue.
 * 5. A preempted process retains its remaining
 *    quantum in its current queue.
 */

#define MLFQ_LEVELS 3

static const int mlfq_quantum[MLFQ_LEVELS] = {
    2, 4, 8
};


/* Queue structure using process indices. */
typedef struct {
    int front;
    int rear;
} MLFQQueue;


/* Add a process at the rear of a queue. */
static void enqueue(
    MLFQQueue *queue,
    int next[],
    int process_index
) {
    next[process_index] = -1;

    if (queue->rear == -1) {
        queue->front = process_index;
        queue->rear = process_index;
    } else {
        next[queue->rear] = process_index;
        queue->rear = process_index;
    }
}


/* Add a preempted process at the front. */
static void enqueue_front(
    MLFQQueue *queue,
    int next[],
    int process_index
) {
    if (queue->front == -1) {
        next[process_index] = -1;
        queue->front = process_index;
        queue->rear = process_index;
    } else {
        next[process_index] = queue->front;
        queue->front = process_index;
    }
}


/* Remove and return the front process. */
static int dequeue(
    MLFQQueue *queue,
    int next[]
) {
    if (queue->front == -1) {
        return -1;
    }

    int process_index = queue->front;

    queue->front = next[process_index];

    if (queue->front == -1) {
        queue->rear = -1;
    }

    next[process_index] = -1;

    return process_index;
}


/* Check whether a queue contains a process. */
static int is_empty(MLFQQueue *queue) {
    return queue->front == -1;
}


/* Return the highest-priority non-empty queue. */
static int highest_ready_queue(MLFQQueue queues[]) {

    for (int level = 0; level < MLFQ_LEVELS; level++) {

        if (!is_empty(&queues[level])) {
            return level;
        }
    }

    return -1;
}


/* Main MLFQ scheduling function. */
int schedule_mlfq(
    Process processes[],
    int process_count
) {

    if (processes == NULL || process_count <= 0) {
        return 0;
    }

    int *next = malloc(
        (size_t)process_count * sizeof(int)
    );

    int *queue_level = malloc(
        (size_t)process_count * sizeof(int)
    );

    int *quantum_used = calloc(
        (size_t)process_count,
        sizeof(int)
    );

    if (next == NULL ||
        queue_level == NULL ||
        quantum_used == NULL) {

        free(next);
        free(queue_level);
        free(quantum_used);

        return 0;
    }


    MLFQQueue queues[MLFQ_LEVELS];

    for (int level = 0; level < MLFQ_LEVELS; level++) {
        queues[level].front = -1;
        queues[level].rear = -1;
    }


    /* Validate and reset all processes. */
    for (int i = 0; i < process_count; i++) {

        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {

            free(next);
            free(queue_level);
            free(quantum_used);

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

        next[i] = -1;
        queue_level[i] = 0;
        quantum_used[i] = 0;
    }


    int time = 0;
    int finished = 0;

    int current_process = -1;
    int current_level = -1;


    while (finished < process_count) {

        /*
         * STEP 1:
         * Add all newly arrived processes to Q0.
         */
        for (int i = 0; i < process_count; i++) {

            if (processes[i].state == PROCESS_NEW &&
                processes[i].arrival_time <= time) {

                queue_level[i] = 0;
                quantum_used[i] = 0;

                processes[i].state = PROCESS_READY;

                enqueue(
                    &queues[0],
                    next,
                    i
                );
            }
        }


        /*
         * STEP 2:
         * Preempt the current process if a higher
         * priority queue has become non-empty.
         */
        if (current_process != -1) {

            int higher_queue_ready = 0;

            for (int level = 0;
                 level < current_level;
                 level++) {

                if (!is_empty(&queues[level])) {
                    higher_queue_ready = 1;
                    break;
                }
            }

            if (higher_queue_ready) {

                processes[current_process].state =
                    PROCESS_READY;

                /*
                 * Preserve its remaining quantum.
                 * Put it at the front of its queue.
                 */
                enqueue_front(
                    &queues[current_level],
                    next,
                    current_process
                );

                current_process = -1;
                current_level = -1;
            }
        }


        /*
         * STEP 3:
         * If CPU is free, select from the highest
         * priority non-empty queue.
         */
        if (current_process == -1) {

            int selected_level =
                highest_ready_queue(queues);

            /*
             * No ready process: jump to the next arrival.
             */
            if (selected_level == -1) {

                int next_arrival = -1;

                for (int i = 0; i < process_count; i++) {

                    if (processes[i].state == PROCESS_NEW) {

                        if (next_arrival == -1 ||
                            processes[i].arrival_time <
                            processes[next_arrival].arrival_time) {

                            next_arrival = i;
                        }
                    }
                }

                if (next_arrival == -1) {
                    break;
                }

                time = processes[next_arrival].arrival_time;

                continue;
            }

            current_process = dequeue(
                &queues[selected_level],
                next
            );

            current_level = selected_level;

            processes[current_process].state =
                PROCESS_RUNNING;

            if (processes[current_process].first_start_time == -1) {

                processes[current_process].first_start_time =
                    time;
            }
        }


        /*
         * STEP 4:
         * Execute the selected process for one time unit.
         */
        Process *p = &processes[current_process];

        p->state = PROCESS_RUNNING;

        p->remaining_time--;

        quantum_used[current_process]++;

        if (!execution_trace_record(p->pid, time, time + 1)) {
            free(next);
            free(queue_level);
            free(quantum_used);
            return 0;
        }
        time++;


        /*
         * STEP 5:
         * Check whether the process has completed.
         */
        if (p->remaining_time == 0) {

            p->completion_time = time;

            p->state = PROCESS_COMPLETED;

            calculate_process_metrics(p);

            finished++;

            current_process = -1;
            current_level = -1;

            continue;
        }


        /*
         * STEP 6:
         * Check whether its time quantum has expired.
         */
        if (quantum_used[current_process] >=
            mlfq_quantum[current_level]) {

            p->state = PROCESS_READY;

            quantum_used[current_process] = 0;

            /*
             * Demote the unfinished process.
             * Q2 is the lowest queue, so it remains
             * in Q2 after using its quantum.
             */
            if (current_level < MLFQ_LEVELS - 1) {

                queue_level[current_process] =
                    current_level + 1;
            }

            int destination =
                queue_level[current_process];

            enqueue(
                &queues[destination],
                next,
                current_process
            );

            current_process = -1;
            current_level = -1;
        }

        /*
         * Otherwise, the process keeps the CPU.
         * The next loop checks for new arrivals and
         * higher-priority preemption.
         */
    }


    free(next);
    free(queue_level);
    free(quantum_used);

    return finished == process_count;
}
