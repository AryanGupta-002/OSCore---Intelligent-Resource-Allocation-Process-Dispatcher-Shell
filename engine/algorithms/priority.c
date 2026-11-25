
#include <stdlib.h>
#include "../include/scheduler.h"
#include "../include/execution_trace.h"
// Priority Scheduling mai jiski priority high hogi wo pehle execute hoga
// Yahan smaller priority number ka matlab higher priority h
// Ye Non-Preemptive h... ek baar CPU mil gaya to process complete hone tak chalega

int schedule_priority( Process processes[], int process_count)
{ // Input sahi nhi h to aage nahi badhenge
    if (processes == NULL || process_count <= 0) {
        return 0;
    }
    // Check karne ke liye array banaya h ki kaunsa process complete hua
    // calloc starting mai sab values 0 kar deta h
    int *completed = calloc(
        (size_t)process_count, sizeof(int)
    );
    if (completed == NULL) {
        return 0;
    }
    // Sabhi processes ki starting values set kr lo
    for (int i = 0; i < process_count; i++) {

        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0) {
            free(completed);
            return 0;
        }
        processes[i].remaining_time =
            processes[i].burst_time;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
        // Abhi tak CPU nhi mila h
        processes[i].first_start_time = -1;
        processes[i].state = PROCESS_NEW;
    }
    int time = 0;
    int finished = 0;
    // Jab tak saare processes complete nhi hote loop chalega
    while (finished < process_count) 
    {int selected = -1;
        // Jo processes aa chuke hain unme highest priority wala find kro
        for (int i = 0; i < process_count; i++) 
        { if (!completed[i] &&
                processes[i].arrival_time <= time) {
                // Smaller priority number wale ko pehle select krenge
                if (selected == -1 ||
                    processes[i].priority <
                    processes[selected].priority) {
                    selected = i;
                }
            }
        }
        // Abhi koi process ready nhi h... next arrival tak wait krenge
        if (selected == -1) {
            int next_arrival = -1;
            // Sabse jaldi aane wale process ko find kar rahe hain
            for (int i = 0; i < process_count; i++) {

                if (!completed[i] &&
                    (next_arrival == -1 ||
                     processes[i].arrival_time <
                     processes[next_arrival].arrival_time)) {

                    next_arrival = i;
                }
            }
            // Seedha next process ke arrival time par chale jao
            time = processes[next_arrival].arrival_time;
            continue;
        }
        Process *p = &processes[selected];
        int start_time = time;
        p->state = PROCESS_RUNNING;
        p->first_start_time = time;
        // Non-preemptive h... process apna pura burst time complete karega
        // Beech mai kisi naye process ke aane se ise rokenge nhi
        time += p->burst_time;
        // Gantt chart ke liye execution record save krna h
        if (!execution_trace_record(p->pid, start_time, time)) {
            free(completed);
            return 0;
        }
        // Process complete ho gaya... ab uski timing update kr do
        p->remaining_time = 0;
        p->completion_time = time;
        p->state = PROCESS_COMPLETED;
        // TAT, WT aur RT calculate ho jayenge
        calculate_process_metrics(p);
        completed[selected] = 1;
        finished++;
    }
    // Kaam complete hone ke baad memory free kr denge
    free(completed);
    return 1;
}
