#include <stdio.h>
#include "process.h"

#define MAX_PROCESSES 100

int main() {

    Process processes[MAX_PROCESSES];
    int count = 0;
    int choice;

    printf("=====================================\n");
    printf("        HOST PROCESS MANAGER         \n");
    printf("=====================================\n");

    printf("\nHow do you want to provide jobs?\n");
    printf("1. Enter jobs manually\n");
    printf("2. Read jobs from file\n");
    printf("3. Exit\n");

    printf("\nEnter your choice: ");
    scanf("%d", &choice);

    switch (choice) {

        case 1:
            printf("\n===== MANUAL JOB INPUT =====\n");

            printf("Enter number of jobs (max %d): ", MAX_PROCESSES);
            scanf("%d", &count);

            if (count <= 0 || count > MAX_PROCESSES) {
                printf("Invalid number of jobs.\n");
                return 1;
            }

            for (int i = 0; i < count; i++) {

                printf("\n--- Job %d ---\n", i + 1);

                printf("PID: ");
                scanf("%d", &processes[i].pid);

                printf("Job Name: ");
                scanf("%14s", processes[i].job);

                printf("Priority: ");
                scanf("%d", &processes[i].priority);

                printf("Arrival Time: ");
                scanf("%d", &processes[i].arrival_time);

                printf("CPU Burst Time: ");
                scanf("%d", &processes[i].burst_time);

                processes[i].remaining_time = processes[i].burst_time;
                processes[i].state = NEW;

                printf("Memory Required (MB): ");
                scanf("%d", &processes[i].memory_required);

                printf("Printers Required: ");
                scanf("%d", &processes[i].printers_required);

                printf("Scanners Required: ");
                scanf("%d", &processes[i].scanners_required);

                printf("Modems Required: ");
                scanf("%d", &processes[i].modems_required);

                printf("Ports Required: ");
                scanf("%d", &processes[i].ports_required);
            }

            break;


        case 2: {
            char filename[100];

            printf("\n===== FILE JOB INPUT =====\n");

            printf("Enter file name: ");
            scanf("%99s", filename);

            FILE *file = fopen(filename, "r");

            if (file == NULL) {
                printf("Error: Could not open file '%s'.\n", filename);
                return 1;
            }

            while (count < MAX_PROCESSES &&
                   fscanf(file, "%d %14s %d %d %d %d %d %d %d %d",
                        &processes[count].pid,
                         processes[count].job,
                        &processes[count].priority,
                        &processes[count].arrival_time,
                        &processes[count].burst_time,
                        &processes[count].memory_required,
                        &processes[count].printers_required,
                        &processes[count].scanners_required,
                        &processes[count].modems_required,
                        &processes[count].ports_required) == 10) {
                         processes[count].remaining_time = processes[count].burst_time;
                         processes[count].state = NEW;

                count++;
            }

            fclose(file);   

            printf("\n%d jobs successfully loaded from file.\n", count);

            break;
        }


        case 3:
            printf("\nExiting HOST...\n");
            return 0;


        default:
            printf("\nInvalid choice.\n");
            return 1;
    }


    /*
     * Display all processes
     */

    printf("\n\n=====================================\n");
    printf("          LOADED PROCESSES           \n");
    printf("=====================================\n");

    for (int i = 0; i < count; i++) {
        print_process(processes[i]);
    }

    return 0;
}
