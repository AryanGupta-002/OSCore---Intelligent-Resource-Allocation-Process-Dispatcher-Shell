//Code by Bibham Bibhuti
#include "../include/scheduler.h"
#include "../include/execution_trace.h"
#include <stddef.h>

// Is scheduling algo mei shortest remaining time wali process run hogi...
int schedule_srtf(
    Process *processes[],
    int process_count
)
{
    if (processes == NULL || process_count <= 0) {
        return 0;
    }

    // Saare processes ke hum initial data ko set kar rahe hai idhar....
    for (int i = 0; i < process_count; i++) {

        if (processes[i]->arrival_time < 0 ||
            processes[i]->burst_time <= 0) {
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

    // Jab tak mere saare process complete nahi hote tb tak meri scheduling run hogi...
    while (finished < process_count) {

        int selected = -1;

        // Jaise jaise mere processes arrive hote rahenge unka hum shortest remaining time find karenge...
        for (int i = 0; i < process_count; i++) {

            if (processes[i]->remaining_time > 0 &&
                processes[i]->arrival_time <= time) {

                if (selected == -1 ||
                    processes[i]->remaining_time <
                    processes[selected]->remaining_time) {

                    selected = i;
                }
            }
        }

        //Agr koi bhi process available nahi hai toh mera CPU process-less rahega.............
        if (selected == -1) {
            time++;
            continue;
        }

        Process *p = processes[selected];

        p->state = PROCESS_RUNNING;

        // jo bhi first execution hoga uska completion time sirf ek baar store karenge...
        if (p->first_start_time == -1) {
            p->first_start_time = time;
        }

        // Ek time unit execute karke hum next process selection dobara check karenge...
        p->remaining_time--;
        time++;

        // Hum yahan pr iss execution ka record maintain kar rahe hai........
        if (!execution_trace_record(p->pid, time - 1, time)) {
            return 0;
        }

        if (p->remaining_time == 0) {

            p->completion_time = time;
            p->state = PROCESS_COMPLETED;

            // Process jab complete hogi uske saare metrics yahi calculate karenege... 
            calculate_process_metrics(p);

            finished++;

        } else {

            // Agr process complete nahi hui toh next scheduling ke turn ke liye ready karenge...
            p->state = PROCESS_READY;
        }
    }

    return 1;
}
