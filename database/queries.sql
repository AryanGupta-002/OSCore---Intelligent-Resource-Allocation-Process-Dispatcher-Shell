-- HOST OS + DBMS Project
-- Analysis, comparison, history, and integrity queries for PostgreSQL.
-- Run from project root:
-- psql -h localhost -U host -d host_db -W -f database/queries.sql
--
-- IMPORTANT:
-- 1. These queries assume the column names match database/schema.sql.
-- 2. Change the example run_id = 1 in queries 3, 5, and 6 to the run you want.
-- 3. For fair algorithm comparison, compare algorithms run against the SAME workload.
-- 4. Analysis is meaningful only when the engine saves real results, metrics,
--    events, snapshots, and state transitions consistently.

-- ============================================================
-- 1. ALL UPLOADED WORKLOADS
-- ============================================================
SELECT
    workload_id,
    workload_name,
    source_filename,
    total_processes,
    created_at
FROM workloads
ORDER BY created_at DESC;


-- ============================================================
-- 2. ALL SIMULATION RUNS WITH WORKLOAD INFORMATION
-- ============================================================
SELECT
    r.run_id,
    w.workload_name,
    r.algorithm,
    r.run_status,
    r.total_execution_time,
    r.cpu_utilization,
    r.completed_processes,
    r.rejected_processes,
    r.started_at,
    r.finished_at
FROM simulation_runs AS r
JOIN workloads AS w ON w.workload_id = r.workload_id
ORDER BY r.run_id DESC;


-- ============================================================
-- 3. PROCESS-WISE RESULTS FOR ONE RUN (change run_id as needed)
-- ============================================================
SELECT
    run_id,
    pid,
    arrival_time,
    burst_time,
    completion_time,
    turnaround_time,
    waiting_time,
    response_time,
    final_state
FROM process_results
WHERE run_id = 1 
--change for other run
ORDER BY pid;


-- ============================================================
-- 4. SAVED PERFORMANCE METRICS FOR EACH RUN
-- ============================================================
SELECT
    r.run_id,
    w.workload_name,
    r.algorithm,
    m.average_turnaround_time,
    m.average_waiting_time,
    m.average_response_time,
    m.cpu_utilization,
    m.throughput,
    m.total_execution_time,
    m.completed_processes,
    m.rejected_processes
FROM performance_metrics AS m
JOIN simulation_runs AS r ON r.run_id = m.run_id
JOIN workloads AS w ON w.workload_id = r.workload_id
ORDER BY w.workload_id, r.run_id;


-- ============================================================
-- 5. EXECUTION EVENT LOG FOR ONE RUN (change run_id as needed)
-- ============================================================
SELECT
    event_id,
    pid,
    event_type,
    start_time,
    end_time,
    priority,
    details
FROM execution_events
WHERE run_id = 1
ORDER BY start_time, event_id;


-- ============================================================
-- 6. RESOURCE SNAPSHOTS FOR ONE RUN (change run_id as needed)
-- ============================================================
SELECT
    simulation_time,
    total_memory_mb,
    used_memory_mb,
    free_memory_mb,
    printers_available,
    scanners_available,
    modems_available,
    cds_available
FROM resource_snapshots
WHERE run_id = 1
ORDER BY simulation_time;


-- ============================================================
-- 7. PROCESS STATE TRANSITIONS FOR ONE RUN (change run_id as needed)
-- ============================================================
SELECT
    pid,
    previous_state,
    new_state,
    simulation_time,
    changed_at,
    details
FROM process_state_history
WHERE run_id = 1
ORDER BY simulation_time, history_id;


-- ============================================================
-- 8. ALGORITHM COMPARISON ONLY FOR WORKLOADS WITH 2+ COMPLETED
-- ALGORITHMS. Each row represents one algorithm's metrics on that workload.
-- ============================================================
SELECT
    w.workload_id,
    w.workload_name,
    r.algorithm,
    m.average_turnaround_time,
    m.average_waiting_time,
    m.average_response_time,
    m.cpu_utilization,
    m.throughput
FROM simulation_runs AS r
JOIN workloads AS w ON w.workload_id = r.workload_id
JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
  AND r.workload_id IN (
      SELECT workload_id
      FROM simulation_runs
      WHERE run_status = 'COMPLETED'
      GROUP BY workload_id
      HAVING COUNT(DISTINCT algorithm) >= 2
  )
ORDER BY w.workload_id, m.average_waiting_time ASC NULLS LAST;


-- ============================================================
-- 9. OVERALL PERFORMANCE BY ALGORITHM
-- This averages per-run averages; workloads may differ, so interpret carefully.
-- ============================================================
SELECT
    r.algorithm,
    COUNT(*) AS total_runs,
    ROUND(AVG(m.average_waiting_time)::numeric, 2) AS avg_waiting_time,
    ROUND(AVG(m.average_turnaround_time)::numeric, 2) AS avg_turnaround_time,
    ROUND(AVG(m.average_response_time)::numeric, 2) AS avg_response_time,
    ROUND(AVG(m.cpu_utilization)::numeric, 2) AS avg_cpu_utilization,
    ROUND(AVG(m.throughput)::numeric, 3) AS avg_throughput
FROM simulation_runs AS r
JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
GROUP BY r.algorithm
ORDER BY avg_waiting_time ASC NULLS LAST;


-- ============================================================
-- 10. BEST ALGORITHM PER WORKLOAD BY LOWEST AVG WAITING TIME
-- Only compares algorithms that completed on the same workload.
-- ============================================================
WITH ranked_algorithms AS (
    SELECT
        w.workload_name,
        r.workload_id,
        r.algorithm,
        m.average_waiting_time,
        m.average_turnaround_time,
        m.average_response_time,
        m.cpu_utilization,
        RANK() OVER (
            PARTITION BY r.workload_id
            ORDER BY m.average_waiting_time ASC NULLS LAST
        ) AS performance_rank
    FROM simulation_runs AS r
    JOIN workloads AS w ON w.workload_id = r.workload_id
    JOIN performance_metrics AS m ON m.run_id = r.run_id
    WHERE r.run_status = 'COMPLETED'
      AND m.average_waiting_time IS NOT NULL
)
SELECT *
FROM ranked_algorithms
WHERE performance_rank = 1
ORDER BY workload_id;


-- ============================================================
-- 11. CPU UTILIZATION, THROUGHPUT, AND EXECUTION TIME BY ALGORITHM
-- This overlaps partly with query 9 but focuses on these 3 metrics.
-- ============================================================
SELECT
    r.algorithm,
    COUNT(*) AS total_runs,
    ROUND(AVG(m.cpu_utilization)::numeric, 2) AS avg_cpu_utilization,
    ROUND(AVG(m.throughput)::numeric, 3) AS avg_throughput,
    ROUND(AVG(m.total_execution_time)::numeric, 2) AS avg_execution_time
FROM simulation_runs AS r
JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
GROUP BY r.algorithm
ORDER BY avg_cpu_utilization DESC NULLS LAST;


-- ============================================================
-- 12. COMPLETED AND REJECTED PROCESS COUNTS PER RUN
-- Assumes each input process has one process_results row, including rejected jobs.
-- ============================================================
SELECT
    r.run_id,
    r.algorithm,
    COUNT(p.result_id) AS total_processes_with_results,
    COUNT(*) FILTER (WHERE p.final_state = 'COMPLETED') AS completed_processes,
    COUNT(*) FILTER (WHERE p.final_state = 'REJECTED') AS rejected_processes,
    ROUND(
        100.0 * COUNT(*) FILTER (WHERE p.final_state = 'REJECTED')
        / NULLIF(COUNT(p.result_id), 0),
        2
    ) AS rejection_percentage
FROM simulation_runs AS r
JOIN process_results AS p ON p.run_id = r.run_id
GROUP BY r.run_id, r.algorithm
ORDER BY r.run_id;


-- ============================================================
-- 13. TOP 10 COMPLETED PROCESSES BY WAITING TIME
-- ============================================================
SELECT
    r.run_id,
    r.algorithm,
    p.pid,
    p.arrival_time,
    p.burst_time,
    p.waiting_time,
    p.turnaround_time,
    p.response_time
FROM process_results AS p
JOIN simulation_runs AS r ON r.run_id = p.run_id
WHERE p.final_state = 'COMPLETED'
  AND p.waiting_time IS NOT NULL
ORDER BY p.waiting_time DESC
LIMIT 10;


-- ============================================================
-- 14. COMPLETE EXECUTION EVENT TIMELINE
-- ============================================================
SELECT
    r.algorithm,
    e.run_id,
    e.start_time,
    e.end_time,
    e.pid,
    e.event_type,
    e.priority,
    e.details
FROM execution_events AS e
JOIN simulation_runs AS r ON r.run_id = e.run_id
ORDER BY e.run_id, e.start_time, e.event_id;


-- ============================================================
-- 15. MEMORY USAGE SUMMARY PER RUN
-- ============================================================
SELECT
    run_id,
    MAX(total_memory_mb) AS total_memory_mb,
    MAX(used_memory_mb) AS peak_memory_mb,
    MIN(free_memory_mb) AS minimum_free_memory_mb,
    ROUND(
        100.0 * MAX(used_memory_mb) / NULLIF(MAX(total_memory_mb), 0),
        2
    ) AS peak_memory_usage_percentage
FROM resource_snapshots
GROUP BY run_id
ORDER BY run_id;


-- ============================================================
-- 16. MINIMUM AVAILABILITY OF I/O DEVICES PER RUN
-- ============================================================
SELECT
    run_id,
    MIN(printers_available) AS minimum_printers_available,
    MIN(scanners_available) AS minimum_scanners_available,
    MIN(modems_available) AS minimum_modems_available,
    MIN(cds_available) AS minimum_cds_available
FROM resource_snapshots
GROUP BY run_id
ORDER BY run_id;


-- ============================================================
-- 17. PROCESS STATE TRANSITION COUNTS
-- ============================================================
SELECT
    run_id,
    new_state,
    COUNT(*) AS number_of_transitions
FROM process_state_history
GROUP BY run_id, new_state
ORDER BY run_id, number_of_transitions DESC;


-- ============================================================
-- 18. COMPLETED RUNS THAT HAVE NO PERFORMANCE METRICS
-- Expected result: zero rows when integration is consistent.
-- ============================================================
SELECT
    r.run_id,
    w.workload_name,
    r.algorithm,
    r.run_status
FROM simulation_runs AS r
JOIN workloads AS w ON w.workload_id = r.workload_id
LEFT JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
  AND m.run_id IS NULL;


-- ============================================================
-- 19. DECLARED PROCESS COUNT VS SAVED PROCESS RESULTS
-- Assumes every input process gets one result row per run, including rejected jobs.
-- ============================================================
SELECT
    r.run_id,
    w.workload_name,
    r.algorithm,
    w.total_processes AS declared_processes,
    COUNT(p.result_id) AS saved_process_results,
    w.total_processes - COUNT(p.result_id) AS difference
FROM simulation_runs AS r
JOIN workloads AS w ON w.workload_id = r.workload_id
LEFT JOIN process_results AS p ON p.run_id = r.run_id
GROUP BY
    r.run_id,
    w.workload_name,
    r.algorithm,
    w.total_processes
ORDER BY r.run_id;


-- ============================================================
-- 20. WORKLOADS THAT HAVE NEVER BEEN SIMULATED
-- ============================================================
SELECT
    w.workload_id,
    w.workload_name,
    w.source_filename,
    w.created_at
FROM workloads AS w
LEFT JOIN simulation_runs AS r ON r.workload_id = w.workload_id
WHERE r.run_id IS NULL
ORDER BY w.created_at DESC;


-- ============================================================
-- 21. RUN COUNTS AND DISTINCT ALGORITHMS PER WORKLOAD
-- ============================================================
SELECT
    w.workload_id,
    w.workload_name,
    COUNT(DISTINCT r.algorithm) AS algorithms_attempted,
    COUNT(r.run_id) AS total_runs,
    COUNT(r.run_id) FILTER (WHERE r.run_status = 'COMPLETED') AS completed_runs,
    COUNT(r.run_id) FILTER (WHERE r.run_status = 'FAILED') AS failed_runs
FROM workloads AS w
LEFT JOIN simulation_runs AS r ON r.workload_id = w.workload_id
GROUP BY w.workload_id, w.workload_name
ORDER BY w.workload_id;


-- ============================================================
-- 22. BEST, WORST, AND AVERAGE WAITING TIME BY ALGORITHM
-- ============================================================
SELECT
    r.algorithm,
    MIN(m.average_waiting_time) AS best_avg_waiting_time,
    MAX(m.average_waiting_time) AS worst_avg_waiting_time,
    ROUND(AVG(m.average_waiting_time)::numeric, 2) AS overall_avg_waiting_time
FROM simulation_runs AS r
JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
GROUP BY r.algorithm
ORDER BY overall_avg_waiting_time ASC NULLS LAST;


-- ============================================================
-- 23. DUPLICATE PROCESS-RESULT CHECK
-- Expected result: zero rows; UNIQUE(run_id, pid) should prevent duplicates.
-- ============================================================
SELECT
    run_id,
    pid,
    COUNT(*) AS record_count
FROM process_results
GROUP BY run_id, pid
HAVING COUNT(*) > 1;


-- ============================================================
-- 24. CHECK FOR MISSING AGGREGATE METRICS
-- Lists completed runs with missing rows or NULL metric values.
-- ============================================================
SELECT
    r.run_id,
    r.algorithm,
    m.average_waiting_time,
    m.average_turnaround_time,
    m.average_response_time,
    m.cpu_utilization,
    m.throughput
FROM simulation_runs AS r
LEFT JOIN performance_metrics AS m ON m.run_id = r.run_id
WHERE r.run_status = 'COMPLETED'
  AND (
      m.run_id IS NULL
      OR m.average_waiting_time IS NULL
      OR m.average_turnaround_time IS NULL
      OR m.average_response_time IS NULL
      OR m.cpu_utilization IS NULL
      OR m.throughput IS NULL
  )
ORDER BY r.run_id;


-- ============================================================
-- 25. RUN COUNTS BY ALGORITHM AND STATUS
-- ============================================================
SELECT
    algorithm,
    run_status,
    COUNT(*) AS run_count
FROM simulation_runs
GROUP BY algorithm, run_status
ORDER BY algorithm, run_status;
