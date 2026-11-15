
#ifndef DB_H
#define DB_H

#include <libpq-fe.h>

/* Establish a connection to PostgreSQL. */
PGconn *db_connect(void);

/* Close the database connection. */
void db_disconnect(PGconn *conn);

/* Check whether the connection is healthy. */
int db_is_connected(PGconn *conn);

long long db_insert_workload(
    PGconn *conn,
    const char *workload_name,
    const char *source_filename,
    int total_processes
);

long long db_create_simulation_run(
    PGconn *conn,
    long long workload_id,
    const char *algorithm,
    int time_quantum
);


int db_finish_simulation_run(
    PGconn *conn,
    long long run_id,
    const char *status,
    int total_execution_time,
    double cpu_utilization,
    int completed_processes,
    int rejected_processes
);


int db_insert_process_result(
    PGconn *conn,
    long long run_id,
    int pid,
    int arrival_time,
    int burst_time,
    int completion_time,
    int turnaround_time,
    int waiting_time,
    int response_time,
    const char *final_state
);


/* Save overall performance metrics for a simulation run. */
int db_insert_performance_metrics(
    PGconn *conn,
    long long run_id,
    double avg_turnaround_time,
    double avg_waiting_time,
    double avg_response_time,
    double cpu_utilization,
    double throughput,
    int total_execution_time,
    int completed_processes,
    int rejected_processes
);

/* Save one execution event. Optional integer fields use -1 for SQL NULL. */
int db_insert_execution_event(
    PGconn *conn,
    long long run_id,
    int pid,
    const char *event_type,
    int start_time,
    int end_time,
    int priority,
    int memory_start_mb,
    int memory_size_mb,
    int printers_available,
    int scanners_available,
    int modems_available,
    int cds_available,
    const char *details
);

/* Save a resource usage snapshot. */
int db_insert_resource_snapshot(
    PGconn *conn,
    long long run_id,
    int simulation_time,
    int total_memory_mb,
    int used_memory_mb,
    int free_memory_mb,
    int printers_available,
    int scanners_available,
    int modems_available,
    int cds_available
);

/* Save a process state transition. NULL previous_state is allowed. */
int db_insert_process_state_history(
    PGconn *conn,
    long long run_id,
    int pid,
    const char *previous_state,
    const char *new_state,
    int simulation_time,
    const char *details
);


#endif
