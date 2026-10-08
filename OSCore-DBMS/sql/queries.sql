-- OSCore DBMS
-- Useful SQL queries for project demonstration


-- 1. Display all jobs

SELECT * FROM jobs;


-- 2. Display all processes

SELECT * FROM processes;


-- 3. JOIN: Jobs with their processes

SELECT
    j.job_id,
    j.job_name,
    p.process_id,
    p.process_state
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id;


-- 4. JOIN: Users and their jobs

SELECT
    u.username,
    u.email,
    j.job_name,
    j.priority
FROM users u
JOIN jobs j
ON u.user_id = j.user_id;


-- 5. JOIN: Process and execution metrics

SELECT
    p.process_id,
    p.process_state,
    e.waiting_time,
    e.turnaround_time,
    e.response_time
FROM processes p
JOIN execution_metrics e
ON p.process_id = e.process_id;


-- 6. Multiple JOIN: User, Job and Process

SELECT
    u.username,
    j.job_name,
    p.process_id,
    p.process_state
FROM users u
JOIN jobs j
ON u.user_id = j.user_id
JOIN processes p
ON j.job_id = p.job_id;


-- 7. Multiple JOIN: Job and resource request

SELECT
    j.job_name,
    p.process_id,
    r.printer_required,
    r.scanner_required,
    r.modem_required,
    r.cd_required
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
JOIN resource_requests r
ON p.process_id = r.process_id;


-- 8. INNER JOIN with execution metrics

SELECT
    j.job_name,
    p.process_state,
    e.waiting_time,
    e.cpu_utilization
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
JOIN execution_metrics e
ON p.process_id = e.process_id;


-- 9. LEFT JOIN: Show all jobs and their processes

SELECT
    j.job_name,
    p.process_id,
    p.process_state
FROM jobs j
LEFT JOIN processes p
ON j.job_id = p.job_id;


-- 10. WHERE: Jobs with priority 1

SELECT *
FROM jobs
WHERE priority = 1;


-- 11. WHERE with multiple conditions

SELECT
    job_name,
    priority,
    cpu_time,
    memory_required
FROM jobs
WHERE priority <= 2
AND cpu_time > 10;


-- 12. BETWEEN

SELECT
    job_name,
    cpu_time
FROM jobs
WHERE cpu_time BETWEEN 10 AND 20;


-- 13. IN

SELECT
    job_name,
    process_state
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
WHERE p.process_state IN ('READY', 'RUNNING');


-- 14. LIKE

SELECT
    job_name
FROM jobs
WHERE job_name LIKE '%er%';


-- 15. ORDER BY

SELECT
    job_name,
    cpu_time
FROM jobs
ORDER BY cpu_time DESC;


-- 16. ORDER BY with LIMIT

SELECT
    job_name,
    cpu_time
FROM jobs
ORDER BY cpu_time DESC
LIMIT 5;


-- 17. COUNT

SELECT COUNT(*) AS total_jobs
FROM jobs;


-- 18. COUNT with condition

SELECT COUNT(*) AS running_processes
FROM processes
WHERE process_state = 'RUNNING';


-- 19. AVG

SELECT
    AVG(waiting_time) AS average_waiting_time
FROM execution_metrics;


-- 20. MAX

SELECT
    MAX(waiting_time) AS maximum_waiting_time
FROM execution_metrics;


-- 21. MIN

SELECT
    MIN(waiting_time) AS minimum_waiting_time
FROM execution_metrics;


-- 22. SUM

SELECT
    SUM(context_switches) AS total_context_switches
FROM execution_metrics;


-- 23. Multiple aggregate functions

SELECT
    COUNT(*) AS total_processes,
    AVG(waiting_time) AS average_waiting_time,
    MAX(waiting_time) AS maximum_waiting_time,
    MIN(waiting_time) AS minimum_waiting_time
FROM execution_metrics;


-- 24. GROUP BY process state

SELECT
    process_state,
    COUNT(*) AS number_of_processes
FROM processes
GROUP BY process_state;


-- 25. GROUP BY priority

SELECT
    priority,
    COUNT(*) AS number_of_jobs
FROM jobs
GROUP BY priority
ORDER BY priority;


-- 26. GROUP BY with AVG

SELECT
    j.priority,
    AVG(e.waiting_time) AS average_waiting_time
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
JOIN execution_metrics e
ON p.process_id = e.process_id
GROUP BY j.priority
ORDER BY j.priority;


-- 27. GROUP BY with multiple aggregates

SELECT
    j.priority,
    COUNT(*) AS total_jobs,
    AVG(e.waiting_time) AS average_waiting_time,
    AVG(e.turnaround_time) AS average_turnaround_time
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
JOIN execution_metrics e
ON p.process_id = e.process_id
GROUP BY j.priority;


-- 28. HAVING

SELECT
    j.priority,
    AVG(e.waiting_time) AS average_waiting_time
FROM jobs j
JOIN processes p
ON j.job_id = p.job_id
JOIN execution_metrics e
ON p.process_id = e.process_id
GROUP BY j.priority
HAVING AVG(e.waiting_time) > 5;


-- 29. HAVING with COUNT

SELECT
    process_state,
    COUNT(*) AS total_processes
FROM processes
GROUP BY process_state
HAVING COUNT(*) > 2;


-- 30. Subquery: jobs above average CPU time

SELECT
    job_name,
    cpu_time
FROM jobs
WHERE cpu_time > (
    SELECT AVG(cpu_time)
    FROM jobs
);


-- 31. Subquery: processes with above-average waiting time

SELECT
    process_id,
    waiting_time
FROM execution_metrics
WHERE waiting_time > (
    SELECT AVG(waiting_time)
    FROM execution_metrics
);


-- 32. CASE statement

SELECT
    job_name,
    cpu_time,
    CASE
        WHEN cpu_time <= 10 THEN 'Low'
        WHEN cpu_time <= 20 THEN 'Medium'
        ELSE 'High'
    END AS cpu_category
FROM jobs;


-- 33. CASE for process state

SELECT
    process_id,
    process_state,
    CASE
        WHEN process_state = 'RUNNING' THEN 'Currently Running'
        WHEN process_state = 'READY' THEN 'Waiting for CPU'
        WHEN process_state = 'NEW' THEN 'New Process'
        WHEN process_state = 'TERMINATED' THEN 'Finished'
        ELSE 'Other'
    END AS state_description
FROM processes;


-- 34. Resource allocation JOIN

SELECT
    p.process_id,
    p.process_state,
    r.printer_allocated,
    r.scanner_allocated,
    r.modem_allocated,
    r.cd_allocated
FROM processes p
JOIN resource_allocations r
ON p.process_id = r.process_id;


-- 35. Process events

SELECT
    p.process_id,
    p.process_state,
    e.event_type,
    e.event_time
FROM processes p
JOIN process_events e
ON p.process_id = e.process_id
ORDER BY p.process_id, e.event_time;


-- 36. Count events for each process

SELECT
    process_id,
    COUNT(*) AS total_events
FROM process_events
GROUP BY process_id
ORDER BY process_id;


-- 37. Scheduling algorithms ordered by waiting time

SELECT
    algorithm,
    average_waiting_time
FROM scheduling_runs
ORDER BY average_waiting_time ASC;


-- 38. Best 3 scheduling results by waiting time

SELECT
    algorithm,
    average_waiting_time,
    average_turnaround_time,
    average_response_time
FROM scheduling_runs
ORDER BY average_waiting_time ASC
LIMIT 3;


-- 39. Scheduling algorithms with high CPU utilization

SELECT
    algorithm,
    cpu_utilization
FROM scheduling_runs
WHERE cpu_utilization > 85
ORDER BY cpu_utilization DESC;


-- 40. Average performance of scheduling algorithms

SELECT
    AVG(average_waiting_time) AS avg_waiting_time,
    AVG(average_turnaround_time) AS avg_turnaround_time,
    AVG(average_response_time) AS avg_response_time,
    AVG(cpu_utilization) AS avg_cpu_utilization
FROM scheduling_runs;


-- 41. GROUP BY algorithm name

SELECT
    algorithm,
    COUNT(*) AS number_of_runs
FROM scheduling_runs
GROUP BY algorithm
ORDER BY algorithm;


-- 42. DISTINCT priorities

SELECT DISTINCT priority
FROM jobs
ORDER BY priority;


-- 43. DISTINCT process states

SELECT DISTINCT process_state
FROM processes;


-- 44. Jobs requiring a printer

SELECT
    job_name,
    printer_required
FROM jobs
WHERE printer_required > 0;


-- 45. Jobs requiring any resource

SELECT
    job_name,
    printer_required,
    scanner_required,
    modem_required,
    cd_required
FROM jobs
WHERE printer_required > 0
   OR scanner_required > 0
   OR modem_required > 0
   OR cd_required > 0;


-- 46. Full project performance report

SELECT
    u.username,
    j.job_name,
    p.process_id,
    p.process_state,
    e.waiting_time,
    e.turnaround_time,
    e.response_time,
    e.cpu_utilization
FROM users u
JOIN jobs j
ON u.user_id = j.user_id
JOIN processes p
ON j.job_id = p.job_id
JOIN execution_metrics e
ON p.process_id = e.process_id
ORDER BY e.waiting_time DESC;