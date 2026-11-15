#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/db.h"

PGconn *db_connect(void)
{
    const char *password = getenv("DB_PASSWORD");
    if (password == NULL || password[0] == '\0') {
        fprintf(stderr, "DB_PASSWORD environment variable is required.\n");
        return NULL;
    }

    const char *keywords[] = {
        "host", "port", "dbname", "user", "password", NULL
    };
    const char *values[] = {
        getenv("DB_HOST") ? getenv("DB_HOST") : "localhost",
        getenv("DB_PORT") ? getenv("DB_PORT") : "5432",
        getenv("DB_NAME") ? getenv("DB_NAME") : "host_db",
        getenv("DB_USER") ? getenv("DB_USER") : "host",
        password,
        NULL
    };

    PGconn *conn = PQconnectdbParams(keywords, values, 0);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "Database connection failed: %s\n",
                PQerrorMessage(conn));

        PQfinish(conn);
        return NULL;
    }

    printf("Connected to PostgreSQL successfully.\n");
    return conn;
}

void db_disconnect(PGconn *conn)
{
    if (conn != NULL)
    {
        PQfinish(conn);
    }
}

int db_is_connected(PGconn *conn)
{
    return conn != NULL &&
           PQstatus(conn) == CONNECTION_OK;
}


long long db_insert_workload(
    PGconn *conn,
    const char *workload_name,
    const char *source_filename,
    int total_processes)
{
    if (!db_is_connected(conn) ||
        workload_name == NULL ||
        total_processes <= 0)
    {
        return -1;
    }

    const char *query =
        "INSERT INTO workloads "
        "(workload_name, source_filename, total_processes) "
        "VALUES ($1, $2, $3) "
        "RETURNING workload_id";

    char process_count[32];
    snprintf(process_count, sizeof(process_count),
             "%d", total_processes);

    const char *values[] = {
        workload_name,
        source_filename,
        process_count
    };

    PGresult *result = PQexecParams(
        conn,
        query,
        3,
        NULL,
        values,
        NULL,
        NULL,
        0
    );

    long long workload_id = -1;

    if (PQresultStatus(result) == PGRES_TUPLES_OK &&
        PQntuples(result) == 1)
    {
        workload_id = atoll(PQgetvalue(result, 0, 0));
    }
    else
    {
        fprintf(stderr, "Workload insertion failed: %s\n",
                PQerrorMessage(conn));
    }

    PQclear(result);
    return workload_id;
}


long long db_create_simulation_run(
    PGconn *conn,
    long long workload_id,
    const char *algorithm,
    int time_quantum)
{
    if (!db_is_connected(conn) ||
        workload_id <= 0 ||
        algorithm == NULL)
    {
        return -1;
    }

    const char *query =
        "INSERT INTO simulation_runs "
        "(workload_id, algorithm, time_quantum, run_status) "
        "VALUES ($1, $2, $3, 'RUNNING') "
        "RETURNING run_id";

    char workload_id_str[32];
    char quantum_str[32];

    snprintf(workload_id_str, sizeof(workload_id_str),
             "%lld", workload_id);

    const char *quantum_value = NULL;

    if (strcmp(algorithm, "RR") == 0)
    {
        if (time_quantum <= 0)
        {
            return -1;
        }

        snprintf(quantum_str, sizeof(quantum_str),
                 "%d", time_quantum);

        quantum_value = quantum_str;
    }

    const char *values[] = {
        workload_id_str,
        algorithm,
        quantum_value
    };

    PGresult *result = PQexecParams(
        conn,
        query,
        3,
        NULL,
        values,
        NULL,
        NULL,
        0
    );

    long long run_id = -1;

    if (PQresultStatus(result) == PGRES_TUPLES_OK &&
        PQntuples(result) == 1)
    {
        run_id = atoll(PQgetvalue(result, 0, 0));
    }
    else
    {
        fprintf(stderr, "Simulation run creation failed: %s\n",
                PQerrorMessage(conn));
    }

    PQclear(result);
    return run_id;
}



int db_finish_simulation_run(
    PGconn *conn,
    long long run_id,
    const char *status,
    int total_execution_time,
    double cpu_utilization,
    int completed_processes,
    int rejected_processes)
{
    if (!db_is_connected(conn) ||
        run_id <= 0 ||
        status == NULL ||
        (strcmp(status, "COMPLETED") != 0 &&
         strcmp(status, "FAILED") != 0) ||
        total_execution_time < 0 ||
        cpu_utilization < 0.0 ||
        cpu_utilization > 100.0 ||
        completed_processes < 0 ||
        rejected_processes < 0)
    {
        return 0;
    }

    const char *query =
        "UPDATE simulation_runs SET "
        "run_status = $1, "
        "total_execution_time = $2, "
        "cpu_utilization = $3, "
        "completed_processes = $4, "
        "rejected_processes = $5, "
        "finished_at = CURRENT_TIMESTAMP "
        "WHERE run_id = $6 AND run_status = 'RUNNING'";

    char execution_time_str[32];
    char cpu_utilization_str[32];
    char completed_str[32];
    char rejected_str[32];
    char run_id_str[32];

    snprintf(execution_time_str, sizeof(execution_time_str),
             "%d", total_execution_time);
    snprintf(cpu_utilization_str, sizeof(cpu_utilization_str),
             "%.2f", cpu_utilization);
    snprintf(completed_str, sizeof(completed_str),
             "%d", completed_processes);
    snprintf(rejected_str, sizeof(rejected_str),
             "%d", rejected_processes);
    snprintf(run_id_str, sizeof(run_id_str),
             "%lld", run_id);

    const char *values[] = {
        status,
        execution_time_str,
        cpu_utilization_str,
        completed_str,
        rejected_str,
        run_id_str
    };

    PGresult *result = PQexecParams(
        conn, query, 6, NULL, values, NULL, NULL, 0
    );

    int success = 0;

    if (PQresultStatus(result) == PGRES_COMMAND_OK &&
        strcmp(PQcmdTuples(result), "1") == 0)
    {
        success = 1;
    }
    else
    {
        fprintf(stderr, "Simulation update failed: %s\n",
                PQerrorMessage(conn));
    }

    PQclear(result);
    return success;
}


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
    const char *final_state)
{
    if (!db_is_connected(conn) ||
        run_id <= 0 ||
        pid <= 0 ||
        arrival_time < 0 ||
        burst_time <= 0 ||
        final_state == NULL ||
        (strcmp(final_state, "COMPLETED") != 0 &&
         strcmp(final_state, "REJECTED") != 0))
    {
        return 0;
    }

    /* Rejected processes may not have completion metrics. */
    int completed = strcmp(final_state, "COMPLETED") == 0;

    if (completed &&
        (completion_time < arrival_time ||
         turnaround_time < 0 ||
         waiting_time < 0 ||
         response_time < 0))
    {
        return 0;
    }

    const char *query =
        "INSERT INTO process_results "
        "(run_id, pid, arrival_time, burst_time, "
        "completion_time, turnaround_time, waiting_time, "
        "response_time, final_state) "
        "VALUES ($1, $2, $3, $4, "
        "$5, $6, $7, $8, $9)";

    char run_id_str[32], pid_str[32], arrival_str[32];
    char burst_str[32], completion_str[32], tat_str[32];
    char waiting_str[32], response_str[32];

    snprintf(run_id_str, sizeof(run_id_str), "%lld", run_id);
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    snprintf(arrival_str, sizeof(arrival_str), "%d", arrival_time);
    snprintf(burst_str, sizeof(burst_str), "%d", burst_time);

    const char *completion_value = NULL;
    const char *tat_value = NULL;
    const char *waiting_value = NULL;
    const char *response_value = NULL;

    if (completed)
    {
        snprintf(completion_str, sizeof(completion_str),
                 "%d", completion_time);
        snprintf(tat_str, sizeof(tat_str), "%d", turnaround_time);
        snprintf(waiting_str, sizeof(waiting_str), "%d", waiting_time);
        snprintf(response_str, sizeof(response_str), "%d", response_time);

        completion_value = completion_str;
        tat_value = tat_str;
        waiting_value = waiting_str;
        response_value = response_str;
    }

    const char *values[] = {
        run_id_str, pid_str, arrival_str, burst_str,
        completion_value, tat_value, waiting_value,
        response_value, final_state
    };

    PGresult *result = PQexecParams(
        conn, query, 9, NULL, values, NULL, NULL, 0
    );

    int success = PQresultStatus(result) == PGRES_COMMAND_OK;

    if (!success)
    {
        fprintf(stderr, "Process result insertion failed: %s\n",
                PQerrorMessage(conn));
    }

    PQclear(result);
    return success;
}



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
    int rejected_processes)
{
    if (!db_is_connected(conn) || run_id <= 0 ||
        avg_turnaround_time < 0 ||
        avg_waiting_time < 0 ||
        avg_response_time < 0 ||
        cpu_utilization < 0 || cpu_utilization > 100 ||
        throughput < 0 || total_execution_time < 0 ||
        completed_processes < 0 || rejected_processes < 0)
        return 0;

    const char *query =
        "INSERT INTO performance_metrics "
        "(run_id, average_turnaround_time, average_waiting_time, "
        "average_response_time, cpu_utilization, throughput, "
        "total_execution_time, completed_processes, rejected_processes) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9)";

    char run[32], tat[32], wt[32], rt[32], cpu[32];
    char tp[32], duration[32], completed[32], rejected[32];

    snprintf(run, sizeof(run), "%lld", run_id);
    snprintf(tat, sizeof(tat), "%.2f", avg_turnaround_time);
    snprintf(wt, sizeof(wt), "%.2f", avg_waiting_time);
    snprintf(rt, sizeof(rt), "%.2f", avg_response_time);
    snprintf(cpu, sizeof(cpu), "%.2f", cpu_utilization);
    snprintf(tp, sizeof(tp), "%.4f", throughput);
    snprintf(duration, sizeof(duration), "%d", total_execution_time);
    snprintf(completed, sizeof(completed), "%d", completed_processes);
    snprintf(rejected, sizeof(rejected), "%d", rejected_processes);

    const char *values[] = {
        run, tat, wt, rt, cpu, tp, duration, completed, rejected
    };

    PGresult *res = PQexecParams(
        conn, query, 9, NULL, values, NULL, NULL, 0
    );

    int ok = PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!ok)
        fprintf(stderr, "Metrics insertion failed: %s\n",
                PQerrorMessage(conn));

    PQclear(res);
    return ok;
}


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
    const char *details)
{
    if (!db_is_connected(conn) || run_id <= 0 ||
        event_type == NULL || event_type[0] == '\0' ||
        start_time < 0 ||
        (end_time >= 0 && end_time < start_time))
        return 0;

    const char *query =
        "INSERT INTO execution_events "
        "(run_id,pid,event_type,start_time,end_time,priority,"
        "memory_start_mb,memory_size_mb,printers_available,"
        "scanners_available,modems_available,cds_available,details) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)";

    char run[32], pid_s[32], start[32], end[32], priority_s[32];
    char mem_start[32], mem_size[32], printers[32], scanners[32];
    char modems[32], cds[32];

    snprintf(run, sizeof(run), "%lld", run_id);
    snprintf(start, sizeof(start), "%d", start_time);

    const char *pid_v = NULL, *end_v = NULL, *priority_v = NULL;
    const char *mem_start_v = NULL, *mem_size_v = NULL;
    const char *printers_v = NULL, *scanners_v = NULL;
    const char *modems_v = NULL, *cds_v = NULL;

    if (pid >= 0) {
        snprintf(pid_s, sizeof(pid_s), "%d", pid);
        pid_v = pid_s;
    }
    if (end_time >= 0) {
        snprintf(end, sizeof(end), "%d", end_time);
        end_v = end;
    }
    if (priority >= 0 && priority <= 3) {
        snprintf(priority_s, sizeof(priority_s), "%d", priority);
        priority_v = priority_s;
    }
    if (memory_start_mb >= 0) {
        snprintf(mem_start, sizeof(mem_start), "%d", memory_start_mb);
        mem_start_v = mem_start;
    }
    if (memory_size_mb >= 0) {
        snprintf(mem_size, sizeof(mem_size), "%d", memory_size_mb);
        mem_size_v = mem_size;
    }
    if (printers_available >= 0) {
        snprintf(printers, sizeof(printers), "%d", printers_available);
        printers_v = printers;
    }
    if (scanners_available >= 0) {
        snprintf(scanners, sizeof(scanners), "%d", scanners_available);
        scanners_v = scanners;
    }
    if (modems_available >= 0) {
        snprintf(modems, sizeof(modems), "%d", modems_available);
        modems_v = modems;
    }
    if (cds_available >= 0) {
        snprintf(cds, sizeof(cds), "%d", cds_available);
        cds_v = cds;
    }

    const char *values[] = {
        run, pid_v, event_type, start, end_v, priority_v,
        mem_start_v, mem_size_v, printers_v, scanners_v,
        modems_v, cds_v, details
    };

    PGresult *res = PQexecParams(
        conn, query, 13, NULL, values, NULL, NULL, 0
    );

    int ok = PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!ok)
        fprintf(stderr, "Event insertion failed: %s\n",
                PQerrorMessage(conn));

    PQclear(res);
    return ok;
}


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
    int cds_available)
{
    if (!db_is_connected(conn) || run_id <= 0 ||
        simulation_time < 0 ||
        total_memory_mb < 0 || used_memory_mb < 0 ||
        free_memory_mb < 0 ||
        used_memory_mb + free_memory_mb != total_memory_mb ||
        printers_available < 0 || scanners_available < 0 ||
        modems_available < 0 || cds_available < 0)
        return 0;

    const char *query =
        "INSERT INTO resource_snapshots "
        "(run_id,simulation_time,total_memory_mb,used_memory_mb,"
        "free_memory_mb,printers_available,scanners_available,"
        "modems_available,cds_available) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9)";

    char run[32], time_s[32], total[32], used[32], free_mem[32];
    char printers[32], scanners[32], modems[32], cds[32];

    snprintf(run, sizeof(run), "%lld", run_id);
    snprintf(time_s, sizeof(time_s), "%d", simulation_time);
    snprintf(total, sizeof(total), "%d", total_memory_mb);
    snprintf(used, sizeof(used), "%d", used_memory_mb);
    snprintf(free_mem, sizeof(free_mem), "%d", free_memory_mb);
    snprintf(printers, sizeof(printers), "%d", printers_available);
    snprintf(scanners, sizeof(scanners), "%d", scanners_available);
    snprintf(modems, sizeof(modems), "%d", modems_available);
    snprintf(cds, sizeof(cds), "%d", cds_available);

    const char *values[] = {
        run, time_s, total, used, free_mem,
        printers, scanners, modems, cds
    };

    PGresult *res = PQexecParams(
        conn, query, 9, NULL, values, NULL, NULL, 0
    );

    int ok = PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!ok)
        fprintf(stderr, "Resource snapshot insertion failed: %s\n",
                PQerrorMessage(conn));

    PQclear(res);
    return ok;
}


int db_insert_process_state_history(
    PGconn *conn,
    long long run_id,
    int pid,
    const char *previous_state,
    const char *new_state,
    int simulation_time,
    const char *details)
{
    if (!db_is_connected(conn) || run_id <= 0 ||
        pid <= 0 || new_state == NULL ||
        simulation_time < 0)
        return 0;

    const char *query =
        "INSERT INTO process_state_history "
        "(run_id,pid,previous_state,new_state,simulation_time,details) "
        "VALUES ($1,$2,$3,$4,$5,$6)";

    char run[32], pid_s[32], time_s[32];

    snprintf(run, sizeof(run), "%lld", run_id);
    snprintf(pid_s, sizeof(pid_s), "%d", pid);
    snprintf(time_s, sizeof(time_s), "%d", simulation_time);

    const char *values[] = {
        run, pid_s, previous_state, new_state, time_s, details
    };

    PGresult *res = PQexecParams(
        conn, query, 6, NULL, values, NULL, NULL, 0
    );

    int ok = PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!ok)
        fprintf(stderr, "State history insertion failed: %s\n",
                PQerrorMessage(conn));

    PQclear(res);
    return ok;
}
