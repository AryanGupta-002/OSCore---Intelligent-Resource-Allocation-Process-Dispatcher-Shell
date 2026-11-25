
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"
// MLFQ mai 3 queues hoti hain... Q0, Q1 aur Q2
// Naya process Q0 mai aata h
// Quantum khatam hone par process next queue mai chala jata h
// Upar wali queue mai process aaya to current process ko rok denge
#define MLFQ_LEVELS 3
// Har queue ka quantum alag h...
// Q0 = 2, Q1 = 4, Q2 = 8
static const int mlfq_quantum[MLFQ_LEVELS] = {
    2, 4, 8
};
// Queue ka front aur rear store karne ke liye
typedef struct {
    int front;
    int rear;
} MLFQQueue;
// Process ko queue ke last mai add krne ke liye
static void enqueue(
    MLFQQueue *queue,
    int next[],
    int process_index
) { next[process_index] = -1;
    // Queue khali h to ye process hi front aur rear dono hoga
    if (queue->rear == -1) {
        queue->front = process_index;
        queue->rear = process_index;
    }
    else {
        // Purane last process ke baad naye process ko jod do
        next[queue->rear] = process_index;
        queue->rear = process_index;
    }
}


// Process ko queue ke starting mai add krne ke liye
// Preemption hone par process ko wapis front mai rakhenge
static void enqueue_front(
    MLFQQueue *queue,
    int next[],
    int process_index
)
{
    if (queue->front == -1) {
        next[process_index] = -1;
        queue->front = process_index;
        queue->rear = process_index;
    }
    else {
        // Naya process ab sabse aage aa jayega
        next[process_index] = queue->front;
        queue->front = process_index;
    }
}


// Queue ke front se process nikalne ke liye
static int dequeue(MLFQQueue *queue, int next[])
{
    if (queue->front == -1)
        return -1;  // Queue khali h to -1 return hoga

    int process_index = queue->front;

    // Front ko next process par shift kr diya
    queue->front = next[process_index];

    // Agar queue mai ab koi nhi bacha to rear bhi reset hoga
    if (queue->front == -1)
        queue->rear = -1;

    next[process_index] = -1;

    return process_index;
}


// Check krna h ki queue khali h ya nhi
static int is_empty(MLFQQueue *queue)
{
    return queue->front == -1;
}


// Highest priority wali ready queue find karne ke liye
// Q0 ki priority sabse high h, isliye pehle wahi check hogi
static int highest_ready_queue(MLFQQueue queues[])
{ for (int level = 0; level < MLFQ_LEVELS; level++) {
        if (!is_empty(&queues[level])) {
            return level;
        }
    }
    return -1;  // Koi bhi queue ready nhi h
}

// Main MLFQ scheduling function
int schedule_mlfq(
    Process processes[],
    int process_count
) {
    // Input galat h to function yahin se return kar dega
    if (processes == NULL || process_count <= 0)
        return 0;

    // 3 arrays banaye hain...
    // next = queue mai next process ka index
    // queue_level = process abhi kis queue mai h
    // quantum_used = process kitna quantum use kar chuka h
    int *next = malloc(
        (size_t)process_count * sizeof(int)
    );

    int *queue_level = malloc(
        (size_t)process_count * sizeof(int)
    );

    // calloc sab values starting mai 0 kar deta h
    int *quantum_used = calloc(
        (size_t)process_count,
        sizeof(int)
    );


    // Memory allocate nhi hui to jo memory mili h use free kr do
    if (next == NULL || queue_level == NULL ||
        quantum_used == NULL) {

        free(next);
        free(queue_level);
        free(quantum_used);
        return 0;
    }

    MLFQQueue queues[MLFQ_LEVELS];
    // Starting mai teeno queues khali hain
    for (int level = 0; level < MLFQ_LEVELS; level++) {
        queues[level].front = -1;
        queues[level].rear = -1;
    }

    // Ab sabhi processes ki initial values set krni hain
    for (int i = 0; i < process_count; i++) {

        // Negative arrival aur 0 burst time valid nhi h
        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            free(next);
            free(queue_level);
            free(quantum_used);
            return 0;
        }

        // Starting mai remaining time burst time ke equal hoga
        processes[i].remaining_time = processes[i].burst_time;

        // Saari timing values ko reset kr diya
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;

        // -1 ka matlab abhi tak CPU nhi mila
        processes[i].first_start_time = -1;

        processes[i].state = PROCESS_NEW;

        next[i] = -1;
        queue_level[i] = 0;
        quantum_used[i] = 0;
    }


    int time = 0;       // Current time
    int finished = 0;   // Kitne processes complete hue

    // -1 ka matlab abhi CPU khali h
    int current_process = -1;
    int current_level = -1;


    // Jab tak saare processes complete nhi hote loop chalega
    while (finished < process_count) {

        // Pehle check kro kaunse processes ka arrival time aa gaya h
        for (int i = 0; i < process_count; i++) {

            if (processes[i].state == PROCESS_NEW &&
                processes[i].arrival_time <= time) {

                // Har naya process Q0 se start karega
                queue_level[i] = 0;
                quantum_used[i] = 0;
                processes[i].state = PROCESS_READY;

                enqueue(&queues[0], next, i);
            }
        }


        // Ab check karenge ki higher priority queue mai koi process h ya nhi
        if (current_process != -1) {
            int higher_queue_ready = 0;
            // Current queue se upar wali queues hi check karni hain
            for (int level = 0; level < current_level; level++) {
                if (!is_empty(&queues[level])) {
                    higher_queue_ready = 1;
                    break;
                }
            }
            if (higher_queue_ready) {
                // Current process ko filhaal rok denge
                processes[current_process].state = PROCESS_READY;
                // Iska bacha hua quantum save rahega...
                // Isliye same queue ke front mai wapis bhejenge
                enqueue_front(
                    &queues[current_level],
                    next,
                    current_process
                );
                current_process = -1;
                current_level = -1;
            }
        }
        // Agar CPU khali h to ab next process select karna h
        if (current_process == -1) {

            // Sabse high priority wali non-empty queue find kro
            int selected_level = highest_ready_queue(queues);
            if (selected_level == -1) {
                // Koi process ready nhi h... CPU idle rahega
                // Ab dekhna h next process kab aayega
                int next_arrival = -1;
                for (int i = 0; i < process_count; i++) {
                    if (processes[i].state == PROCESS_NEW) {
                        // Sabse jaldi aane wale process ko find karenge
                        if (next_arrival == -1 ||
                            processes[i].arrival_time <
                            processes[next_arrival].arrival_time) {
                            next_arrival = i;
                        }
                    } }

                // Koi process bacha hi nhi to loop end
                if (next_arrival == -1)
                    break;
                // Seedha next process ke arrival time par chale jao
                time = processes[next_arrival].arrival_time;

                continue;
            }

            // Selected queue ke front se process nikal lo
            current_process = dequeue(
                &queues[selected_level],
                next
            );
            current_level = selected_level;
            processes[current_process].state = PROCESS_RUNNING;
            // Pehli baar CPU mila h to starting time save kr lo
            if (processes[current_process].first_start_time == -1) {
                processes[current_process].first_start_time = time;
            }
        }


        // Ab selected process ko 1 time unit ke liye chalayenge
        Process *p = &processes[current_process];

        p->state = PROCESS_RUNNING;
        p->remaining_time--;
        quantum_used[current_process]++;

        // Gantt chart mai process ka execution record save hoga
        if (!execution_trace_record(p->pid, time, time + 1)) {
            free(next);
            free(queue_level);
            free(quantum_used);
            return 0;
        }

        time++;

        // Agar remaining time 0 ho gaya to process complete h
        if (p->remaining_time == 0) {
            p->completion_time = time;
            p->state = PROCESS_COMPLETED;
            // Completion ke baad TAT, WT aur RT calculate honge
            calculate_process_metrics(p);
            finished++;
            current_process = -1;
            current_level = -1;
            continue;
        }

        // Quantum khatam ho gaya but process ka kaam abhi baaki h
        if (quantum_used[current_process] >=
            mlfq_quantum[current_level]) {

            p->state = PROCESS_READY;

            // Next turn mai fresh quantum milega
            quantum_used[current_process] = 0;

            // Q0 se Q1 aur Q1 se Q2 mai bhej denge
            // Q2 last queue h... isliye process wahi rahega
            if (current_level < MLFQ_LEVELS - 1) 
            {queue_level[current_process] = current_level + 1;
            }
            int destination = queue_level[current_process];
            // Ab process ko next queue ke last mai add kr do
            enqueue(
                &queues[destination],
                next,
                current_process
            );
            current_process = -1;
            current_level = -1;
        }

        // Agar quantum bacha h to same process CPU par continue karega
    }
    // Saara kaam hone ke baad allocated memory free kr denge
    free(next);
    free(queue_level);
    free(quantum_used);
    return finished == process_count;
}
