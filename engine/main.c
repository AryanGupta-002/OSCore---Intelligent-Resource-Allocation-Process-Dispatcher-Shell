#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>

#include "include/process.h"
#include "include/scheduler.h"
#include "include/execution_trace.h"
#include "include/db.h"
#include "host/host.h"

#define MAX_PROCESSES 1000
#define MAX_PATH_LENGTH 512
#define MAX_WORKLOAD_FILES 100
#define INPUT_LENGTH 512

typedef struct {
    int total_execution_time;
    int completed;
    int rejected;
    double cpu_utilization;
    double avg_turnaround_time;
    double avg_waiting_time;
    double avg_response_time;
    double throughput;
} RunMetrics;

static const char *database_algorithm_name(const char *algorithm)
{
    if (strcmp(algorithm, "fcfs") == 0) return "FCFS";
    if (strcmp(algorithm, "sjf") == 0) return "SJF";
    if (strcmp(algorithm, "priority") == 0) return "PRIORITY";
    if (strcmp(algorithm, "rr") == 0) return "RR";
    if (strcmp(algorithm, "srtf") == 0) return "SRTF";
    if (strcmp(algorithm, "mlfq") == 0) return "MLFQ";
    if (strcmp(algorithm, "host") == 0) return "HOST";
    
    return NULL;
}

/* Read and validate a numeric menu choice. Returns 1 on success, 0 on EOF. */
static int read_integer(const char *prompt, int *result)
{
    char input[INPUT_LENGTH];
    char *end;
    long value;

    printf("%s", prompt);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    value = strtol(input, &end, 10);
    while (isspace((unsigned char)*end)) end++;

    if (end == input || *end != '\0' || value < -2147483647L - 1L ||
        value > 2147483647L) {
        printf("Invalid input. Please enter a valid integer.\n");
        return -1;
    }

    *result = (int)value;
    return 1;
}

static int has_csv_extension(const char *name)
{
    size_t length = strlen(name);
    return length >= 4 &&
           tolower((unsigned char)name[length - 4]) == '.' &&
           tolower((unsigned char)name[length - 3]) == 'c' &&
           tolower((unsigned char)name[length - 2]) == 's' &&
           tolower((unsigned char)name[length - 1]) == 'v';
}

static int select_workload(char selected_path[], size_t selected_size)
{
    char files[MAX_WORKLOAD_FILES][MAX_PATH_LENGTH];
    int file_count = 0;
    DIR *directory = opendir("data");
    struct dirent *entry;
    int choice;
    int status;

    if (directory != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (!has_csv_extension(entry->d_name)) continue;
            if (file_count >= MAX_WORKLOAD_FILES) break;

            snprintf(files[file_count], sizeof(files[file_count]),
                     "data/%s", entry->d_name);
            file_count++;
        }
        closedir(directory);
    }

    printf("\n==================================================\n");
    printf("       OSCORE - WORKLOAD SELECTION\n");
    printf("==================================================\n");

    for (int i = 0; i < file_count; i++) {
        printf("%d. %s\n", i + 1, files[i]);
    }
    printf("%d. Enter a custom CSV file path\n", file_count + 1);
    printf("%d. Exit\n", file_count + 2);

    status = read_integer("\nChoose a workload: ", &choice);
    if (status == 0) return 0;
    if (status < 0) return -1;

    if (choice >= 1 && choice <= file_count) {
        snprintf(selected_path, selected_size, "%s", files[choice - 1]);
        return 1;
    }

    if (choice == file_count + 1) {
        char input[MAX_PATH_LENGTH];
        printf("Enter CSV path: ");
        if (fgets(input, sizeof(input), stdin) == NULL) return 0;
        input[strcspn(input, "\r\n")] = '\0';

        if (input[0] == '\0') {
            printf("File path cannot be empty.\n");
            return -1;
        }

        snprintf(selected_path, selected_size, "%s", input);
        return 1;
    }

    if (choice == file_count + 2) return 0;

    printf("Invalid workload choice.\n");
    return -1;
}

static RunMetrics calculate_metrics(Process processes[], int process_count)
{
    RunMetrics metrics = {0};
    double total_tat = 0.0;
    double total_wt = 0.0;
    double total_rt = 0.0;
    double completed_burst = 0.0;

    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];

        if (p->completion_time > metrics.total_execution_time) {
            metrics.total_execution_time = p->completion_time;
        }

        if (p->state == PROCESS_COMPLETED) {
            metrics.completed++;
            total_tat += p->turnaround_time;
            total_wt += p->waiting_time;
            total_rt += p->response_time;
            completed_burst += p->burst_time;
        } else {
            metrics.rejected++;
        }
    }

    if (metrics.completed > 0) {
        metrics.avg_turnaround_time = total_tat / metrics.completed;
        metrics.avg_waiting_time = total_wt / metrics.completed;
        metrics.avg_response_time = total_rt / metrics.completed;
    }

    if (metrics.total_execution_time > 0) {
        metrics.cpu_utilization =
            completed_burst / metrics.total_execution_time * 100.0;
        metrics.throughput =
            (double)metrics.completed / metrics.total_execution_time;
    }

    return metrics;
}

static void print_results(Process processes[], int process_count,
                          const RunMetrics *metrics)
{
    printf("\nPID\tAT\tBT\tCT\tTAT\tWT\tRT\tSTATE\n");

    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        const char *state = p->state == PROCESS_COMPLETED ? "COMPLETED" : "REJECTED";

        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
               p->pid, p->arrival_time, p->burst_time,
               p->completion_time, p->turnaround_time,
               p->waiting_time, p->response_time, state);
    }

    printf("\n--------------- PERFORMANCE SUMMARY ---------------\n");
    printf("Total execution time       : %d\n", metrics->total_execution_time);
    printf("CPU utilization            : %.2f%%\n", metrics->cpu_utilization);
    printf("Average turnaround time    : %.2f\n", metrics->avg_turnaround_time);
    printf("Average waiting time       : %.2f\n", metrics->avg_waiting_time);
    printf("Average response time      : %.2f\n", metrics->avg_response_time);
    printf("Throughput                 : %.4f processes/unit time\n", metrics->throughput);
    printf("Completed processes        : %d\n", metrics->completed);
    printf("Rejected/incomplete        : %d\n", metrics->rejected);
}

static void print_host_events(const Host *host)
{
    printf("\n---------------- HOST EXECUTION EVENTS ----------------\n");
    printf("Time\tPID\tEvent\t\t\tDetails\n");

    for (int i = 0; i < host->event_log.count; i++) {
        const ExecutionEvent *event = &host->event_log.events[i];
        printf("%d-%d\t", event->start_time, event->end_time);
        if (event->pid <= 0) printf("-\t");
        else printf("%d\t", event->pid);
        printf("%-24s\t%s\n", event_type_name(event->type), event->details);
    }
}

static int save_host_events(PGconn *conn, long long run_id, const Host *host)
{
    if (conn == NULL || host == NULL) return 0;

    for (int i = 0; i < host->event_log.count; i++) {
        const ExecutionEvent *event = &host->event_log.events[i];
        if (!db_insert_execution_event(
                conn, run_id, event->pid > 0 ? event->pid : -1,
                event_type_name(event->type), event->start_time,
                event->end_time, event->priority, event->memory_start_mb,
                event->memory_size_mb, event->printers_available,
                event->scanners_available, event->modems_available,
                event->cds_available, event->details)) {
            fprintf(stderr, "Failed to save HOST event %d (PID %d).\n",
                    i, event->pid);
            return 0;
        }
    }
    return 1;
}

static int save_process_results(PGconn *conn, long long run_id,
                                Process processes[], int process_count)
{
    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        const char *state = p->state == PROCESS_COMPLETED ? "COMPLETED" : "REJECTED";

        if (!db_insert_process_result(
                conn, run_id, p->pid, p->arrival_time, p->burst_time,
                p->completion_time, p->turnaround_time, p->waiting_time,
                p->response_time, state)) {
            fprintf(stderr, "Failed to save result for PID %d.\n", p->pid);
            return 0;
        }
    }
    return 1;
}

/* Runs one algorithm on a fresh copy of the selected workload. */
static int execute_algorithm(PGconn *conn, long long workload_id,
                             const Process base_processes[], int process_count,
                             const char *algorithm, int time_quantum,
                             const HostConfig *host_config,
                             RunMetrics *metrics_out)
{
    Process processes[MAX_PROCESSES];
    const char *db_algorithm = database_algorithm_name(algorithm);
    long long run_id;
    int success = 0;
    int host_initialized = 0;
    Host host;
    RunMetrics metrics;
    int saved;

    if (process_count <= 0 || process_count > MAX_PROCESSES || db_algorithm == NULL) {
        fprintf(stderr, "Invalid process count or algorithm.\n");
        return 0;
    }

    memcpy(processes, base_processes, (size_t)process_count * sizeof(Process));

    run_id = db_create_simulation_run(conn, workload_id, db_algorithm, time_quantum);
    if (run_id < 0) {
        fprintf(stderr, "Could not create database record for %s.\n", db_algorithm);
        return 0;
    }

    printf("\n==================================================\n");
    printf("Running algorithm: %s\n", db_algorithm);
    if (strcmp(algorithm, "rr") == 0) printf("Time quantum: %d\n", time_quantum);
    printf("Simulation Run ID: %lld\n", run_id);
    printf("OSCORE_RUN_ID=%lld\n", run_id);
    printf("==================================================\n");

    execution_trace_reset();
    if (strcmp(algorithm, "fcfs") == 0) {
        success = schedule_fcfs(processes, process_count);
    } else if (strcmp(algorithm, "sjf") == 0) {
        success = schedule_sjf(processes, process_count);
    } else if (strcmp(algorithm, "priority") == 0) {
        success = schedule_priority(processes, process_count);
    } else if (strcmp(algorithm, "rr") == 0) {
        success = schedule_round_robin(processes, process_count, time_quantum);
    } else if (strcmp(algorithm, "srtf") == 0) {
        success = schedule_srtf(processes, process_count);
        
    } else if (strcmp(algorithm, "mlfq") == 0) {
        success = schedule_mlfq(processes, process_count);
    } else if (strcmp(algorithm, "host") == 0) {
        if (initialize_host(&host, host_config)) {
            host_initialized = 1;
            success = run_host(&host, processes, process_count);
            if (success) print_host_events(&host);
        } else {
            fprintf(stderr, "Failed to initialize HOST.\n");
        }
    }

    if (!success) {
        fprintf(stderr, "%s simulation failed.\n", db_algorithm);
        db_finish_simulation_run(conn, run_id, "FAILED", 0, 0.0, 0, process_count);
        if (host_initialized) destroy_host(&host);
        return 0;
    }

    metrics = calculate_metrics(processes, process_count);
    if (metrics_out != NULL) *metrics_out = metrics;
    print_results(processes, process_count, &metrics);

    saved = save_process_results(conn, run_id, processes, process_count);
    if (saved && host_initialized) saved = save_host_events(conn, run_id, &host);
    if (saved && !host_initialized) {
        for (size_t i = 0; i < execution_trace_count(); i++) {
            const ExecutionTraceSegment *segment = execution_trace_get(i);
            if (!segment || !db_insert_execution_event(
                    conn,
                    run_id,
                    segment->pid,
                    "CPU_EXECUTION",
                    segment->start_time,
                    segment->end_time,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    "Scheduled CPU interval")) {
                saved = 0;
                break;
            }
        }
    }
    if (saved) {
        saved = db_insert_performance_metrics(
            conn, run_id, metrics.avg_turnaround_time,
            metrics.avg_waiting_time, metrics.avg_response_time,
            metrics.cpu_utilization, metrics.throughput,
            metrics.total_execution_time, metrics.completed, metrics.rejected);
    }

    if (!saved) {
        fprintf(stderr, "Could not save all results for run %lld.\n", run_id);
        db_finish_simulation_run(conn, run_id, "FAILED",
                                 metrics.total_execution_time,
                                 metrics.cpu_utilization,
                                 metrics.completed, metrics.rejected);
        if (host_initialized) destroy_host(&host);
        return 0;
    }

    if (!db_finish_simulation_run(conn, run_id, "COMPLETED",
                                  metrics.total_execution_time,
                                  metrics.cpu_utilization,
                                  metrics.completed, metrics.rejected)) {
        fprintf(stderr, "Could not mark run %lld as COMPLETED.\n", run_id);
        if (host_initialized) destroy_host(&host);
        return 0;
    }

    printf("\n%s simulation completed and results saved to PostgreSQL.\n", db_algorithm);
    if (host_initialized) destroy_host(&host);
    return 1;
}

static int show_simulation_menu(void)
{
    int choice;
    int status;

    printf("\n==================================================\n");
    printf("       OSCORE - SIMULATION MENU\n");
    printf("==================================================\n");
    printf("1. Run HOST Simulation\n");
    printf("2. Run FCFS\n");
    printf("3. Run SJF\n");
    printf("4. Run Priority Scheduling\n");
    printf("5. Run Round Robin\n");
    printf("6. Run SRTF\n");
    printf("7. Run MLFQ\n");
    printf("8. Compare All Six Scheduling Algorithms\n");
    printf("9. Change Workload\n");
    printf("10. Exit\n");

    status = read_integer("\nChoose an option: ", &choice);
    if (status <= 0) return status;
    if (choice < 1 || choice > 10) {
        printf("Invalid option. Choose 1-10.\n");
        return -1;
    }
    return choice;
}

static int run_cli(int argc, char *argv[])
{
    char *end;
    long quantum_value;
    int time_quantum;
    PGconn *conn;
    Process processes[MAX_PROCESSES];
    int process_count;
    long long workload_id;
    int success;
    HostConfig host_config;
    const HostConfig *host_config_ptr = NULL;

    if (argc != 5 && argc != 10) {
        fprintf(stderr,
                "Usage: %s <workload.csv> <algorithm> <time_quantum> <workload_name> "
                "[total_memory_mb printers scanners modems cds]\n",
                argv[0]);
        return 2;
    }

    if (database_algorithm_name(argv[2]) == NULL) {
        fprintf(stderr, "Unsupported algorithm: %s\n", argv[2]);
        return 2;
    }

    if (argc == 10) {
        if (strcmp(argv[2], "host") != 0) {
            fprintf(stderr, "Machine configuration is only valid for HOST.\n");
            return 2;
        }

        int *config_values[] = {
            &host_config.total_memory_mb,
            &host_config.printers,
            &host_config.scanners,
            &host_config.modems,
            &host_config.cds
        };
        for (int i = 0; i < 5; i++) {
            long value = strtol(argv[i + 5], &end, 10);
            if (end == argv[i + 5] || *end != '\0' ||
                value < 0 || value > INT_MAX) {
                fprintf(stderr, "Machine capacities must be non-negative integers.\n");
                return 2;
            }
            *config_values[i] = (int)value;
        }
        if (host_config.total_memory_mb < 2) {
            fprintf(stderr, "Machine memory must be at least 2 MB.\n");
            return 2;
        }
        host_config_ptr = &host_config;
    }

    quantum_value = strtol(argv[3], &end, 10);
    if (end == argv[3] || *end != '\0' || quantum_value < 0 ||
        quantum_value > INT_MAX) {
        fprintf(stderr, "Time quantum must be a non-negative integer.\n");
        return 2;
    }
    time_quantum = (int)quantum_value;

    if (strcmp(argv[2], "rr") == 0 && time_quantum <= 0) {
        fprintf(stderr, "Round Robin requires a positive time quantum.\n");
        return 2;
    }

    conn = db_connect();
    if (!db_is_connected(conn)) {
        fprintf(stderr, "Database connection failed. Check PostgreSQL settings.\n");
        db_disconnect(conn);
        return 1;
    }

    process_count = load_processes(argv[1], processes, MAX_PROCESSES);
    if (process_count <= 0) {
        fprintf(stderr, "Could not load workload '%s'.\n", argv[1]);
        db_disconnect(conn);
        return 1;
    }

    workload_id = db_insert_workload(
        conn, argv[4], argv[1], process_count
    );
    if (workload_id < 0) {
        fprintf(stderr, "Could not save workload metadata to PostgreSQL.\n");
        db_disconnect(conn);
        return 1;
    }

    success = execute_algorithm(
        conn, workload_id, processes, process_count,
        argv[2], time_quantum, host_config_ptr, NULL
    );
    db_disconnect(conn);
    return success ? 0 : 1;
}

int main(int argc, char *argv[])
{
    PGconn *conn;

    if (argc > 1) {
        return run_cli(argc, argv);
    }

    printf("\n==================================================\n");
    printf(" OSCORE - Intelligent Resource Allocation &\n");
    printf(" Process Dispatcher Shell\n");
    printf("==================================================\n");

    conn = db_connect();
    if (!db_is_connected(conn)) {
        fprintf(stderr, "Database connection failed. Check PostgreSQL settings.\n");
        db_disconnect(conn);
        return 1;
    }
    printf("Connected to PostgreSQL.\n");

    for (;;) {
        char csv_file[MAX_PATH_LENGTH];
        Process base_processes[MAX_PROCESSES];
        int process_count;
        long long workload_id;
        int menu_choice;
        int status;

        status = select_workload(csv_file, sizeof(csv_file));
        if (status == 0) break;
        if (status < 0) continue;

        process_count = load_processes(csv_file, base_processes, MAX_PROCESSES);
        if (process_count <= 0) {
            fprintf(stderr, "Could not load workload '%s'. Try another file.\n", csv_file);
            continue;
        }

        printf("\nSelected workload: %s\n", csv_file);
        printf("Processes loaded: %d\n", process_count);

        workload_id = db_insert_workload(conn, csv_file, csv_file, process_count);
        if (workload_id < 0) {
            fprintf(stderr, "Could not save workload metadata to PostgreSQL.\n");
            continue;
        }
        printf("Workload ID: %lld\n", workload_id);

        for (;;) {
            int time_quantum = 0;
            const char *algorithm = NULL;

            menu_choice = show_simulation_menu();
            if (menu_choice == 0) goto cleanup;
            if (menu_choice < 0) continue;
            if (menu_choice == 10) goto cleanup;
            if (menu_choice == 9) break;

            switch (menu_choice) {
                case 1: algorithm = "host"; break;
                case 2: algorithm = "fcfs"; break;
                case 3: algorithm = "sjf"; break;
                case 4: algorithm = "priority"; break;
                case 5: algorithm = "rr"; break;
                case 6: algorithm = "srtf"; break;
                case 7: algorithm = "mlfq"; break;
                default: break;
            }

            if (menu_choice == 8) {
                status = read_integer("Round Robin quantum for comparison (e.g. 2): ",
                                      &time_quantum);
                if (status == 0) goto cleanup;
                if (status < 0 || time_quantum <= 0) {
                    printf("Quantum must be a positive integer.\n");
                    continue;
                }

                const char *compare_algorithms[] = {
                    "fcfs", "sjf", "srtf", "priority", "rr", "mlfq"
                };
                RunMetrics compare_metrics[6] = {{0}};
                int compare_success[6] = {0};

                printf("\nComparing all six scheduling algorithms on the same workload.\n");
                printf("Each algorithm receives a fresh copy of the input processes.\n");
                for (int i = 0; i < 6; i++) {
                    compare_success[i] = execute_algorithm(
                        conn, workload_id, base_processes, process_count,
                        compare_algorithms[i],
                        strcmp(compare_algorithms[i], "rr") == 0 ? time_quantum : 0,
                        NULL,
                        &compare_metrics[i]);
                }

                printf("\n================ ALGORITHM COMPARISON ================\n");
                printf("Algorithm\tAvg TAT\tAvg WT\tAvg RT\tCPU Util%%\tThroughput\tStatus\n");
                for (int i = 0; i < 6; i++) {
                    if (compare_success[i]) {
                        printf("%-10s\t%.2f\t\t%.2f\t%.2f\t%.2f\t\t%.4f\t\tCOMPLETED\n",
                               database_algorithm_name(compare_algorithms[i]),
                               compare_metrics[i].avg_turnaround_time,
                               compare_metrics[i].avg_waiting_time,
                               compare_metrics[i].avg_response_time,
                               compare_metrics[i].cpu_utilization,
                               compare_metrics[i].throughput);
                    } else {
                        printf("%-10s\t-\t\t-\t-\t-\t\t-\t\tFAILED\n",
                               database_algorithm_name(compare_algorithms[i]));
                    }
                }
                printf("\nComparison runs have been saved to PostgreSQL where successful.\n");
                continue;
            }

            if (algorithm != NULL) {
                if (strcmp(algorithm, "rr") == 0) {
                    status = read_integer("Enter Round Robin time quantum: ", &time_quantum);
                    if (status == 0) goto cleanup;
                    if (status < 0 || time_quantum <= 0) {
                        printf("Quantum must be a positive integer.\n");
                        continue;
                    }
                }
                execute_algorithm(conn, workload_id, base_processes,
                                  process_count, algorithm, time_quantum, NULL, NULL);
            }
        }

        /* The outer loop returns to workload selection when requested. */
    }

cleanup:
    db_disconnect(conn);
    printf("\nOSCORE simulator closed. Goodbye!\n");
    return 0;
}
