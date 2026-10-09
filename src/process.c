#include <stdio.h>
#include "process.h"


void print_process(Process p) {

    printf("\n----------------------------------------\n");

    printf("PID               : %d\n", p.pid);
    printf("Job               : %s\n", p.job);
    printf("Priority          : %d\n", p.priority);
    printf("Arrival Time      : %d\n", p.arrival_time);
    printf("CPU Burst Time    : %d\n", p.burst_time);
    printf("Remaining Time    : %d\n", p.remaining_time);
    printf("Memory Required   : %d MB\n", p.memory_required);

    printf("Printers Required : %d\n", p.printers_required);
    printf("Scanners Required : %d\n", p.scanners_required);
    printf("Modems Required   : %d\n", p.modems_required);
    printf("Ports Required    : %d\n", p.ports_required);

    printf("State             : ");

    switch (p.state) {

        case NEW:
            printf("NEW");
            break;

        case READY:
            printf("READY");
            break;

        case RUNNING:
            printf("RUNNING");
            break;

        case WAITING:
            printf("WAITING");
            break;

        case TERMINATED:
            printf("TERMINATED");
            break;

        default:
            printf("UNKNOWN");
    }

    printf("\n----------------------------------------\n");
}
