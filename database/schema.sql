-- HOST OS-DBMS Project
-- Database: host_db
-- Creates the tables required for workloads,
-- simulation runs, process results and performance analysis.

BEGIN;

CREATE TABLE IF NOT EXISTS workloads (
    workload_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    workload_name VARCHAR(100) NOT NULL,
    source_filename VARCHAR(255),
    total_processes INT NOT NULL CHECK (total_processes > 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE IF NOT EXISTS simulation_runs (
    run_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    workload_id BIGINT NOT NULL
        REFERENCES workloads(workload_id)
        ON DELETE CASCADE,

    algorithm VARCHAR(20) NOT NULL
        CHECK (algorithm IN (
            'FCFS',
            'SJF',
            'PRIORITY',
            'RR',
            'SRTF',
            'MLFQ',
            'HOST'
        )),

    time_quantum INT,

    run_status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
        CHECK (run_status IN (
            'PENDING',
            'RUNNING',
            'COMPLETED',
            'FAILED'
        )),

    total_execution_time INT
        CHECK (total_execution_time >= 0),

    cpu_utilization NUMERIC(5,2)
        CHECK (cpu_utilization BETWEEN 0 AND 100),

    completed_processes INT NOT NULL DEFAULT 0
        CHECK (completed_processes >= 0),

    rejected_processes INT NOT NULL DEFAULT 0
        CHECK (rejected_processes >= 0),

    started_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    finished_at TIMESTAMPTZ
);

ALTER TABLE simulation_runs
    DROP CONSTRAINT IF EXISTS simulation_runs_algorithm_check;

ALTER TABLE simulation_runs
    ADD CONSTRAINT simulation_runs_algorithm_check
    CHECK (algorithm IN (
        'FCFS',
        'SJF',
        'PRIORITY',
        'RR',
        'SRTF',
        'MLFQ',
        'HOST'
    ));


CREATE TABLE IF NOT EXISTS process_results (
    result_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    run_id BIGINT NOT NULL
        REFERENCES simulation_runs(run_id)
        ON DELETE CASCADE,

    pid INT NOT NULL,
    arrival_time INT NOT NULL CHECK (arrival_time >= 0),
    burst_time INT NOT NULL CHECK (burst_time > 0),

    completion_time INT,
    turnaround_time INT,
    waiting_time INT,
    response_time INT,

    final_state VARCHAR(20) NOT NULL
        CHECK (final_state IN (
            'COMPLETED',
            'REJECTED'
        )),

    UNIQUE (run_id, pid)
);


CREATE TABLE IF NOT EXISTS execution_events (
    event_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    run_id BIGINT NOT NULL
        REFERENCES simulation_runs(run_id)
        ON DELETE CASCADE,

    pid INT,
    event_type VARCHAR(40) NOT NULL,

    start_time INT NOT NULL CHECK (start_time >= 0),
    end_time INT CHECK (end_time >= start_time),

    priority INT CHECK (priority BETWEEN 0 AND 3),

    memory_start_mb INT,
    memory_size_mb INT,

    printers_available INT,
    scanners_available INT,
    modems_available INT,
    cds_available INT,

    details TEXT
);


CREATE TABLE IF NOT EXISTS resource_snapshots (
    snapshot_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    run_id BIGINT NOT NULL
        REFERENCES simulation_runs(run_id)
        ON DELETE CASCADE,

    simulation_time INT NOT NULL
        CHECK (simulation_time >= 0),

    total_memory_mb INT NOT NULL,
    used_memory_mb INT NOT NULL,
    free_memory_mb INT NOT NULL,

    printers_available INT NOT NULL,
    scanners_available INT NOT NULL,
    modems_available INT NOT NULL,
    cds_available INT NOT NULL,

    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CHECK (used_memory_mb + free_memory_mb = total_memory_mb)
);


CREATE TABLE IF NOT EXISTS performance_metrics (
    metric_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    run_id BIGINT NOT NULL
        REFERENCES simulation_runs(run_id)
        ON DELETE CASCADE,

    average_turnaround_time NUMERIC(10,2),
    average_waiting_time NUMERIC(10,2),
    average_response_time NUMERIC(10,2),

    cpu_utilization NUMERIC(5,2),
    throughput NUMERIC(10,4),

    total_execution_time INT,
    completed_processes INT NOT NULL DEFAULT 0,
    rejected_processes INT NOT NULL DEFAULT 0,

    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,

    UNIQUE (run_id),

    CHECK (cpu_utilization IS NULL OR
           cpu_utilization BETWEEN 0 AND 100),

    CHECK (average_turnaround_time IS NULL OR
           average_turnaround_time >= 0),

    CHECK (average_waiting_time IS NULL OR
           average_waiting_time >= 0),

    CHECK (average_response_time IS NULL OR
           average_response_time >= 0),

    CHECK (completed_processes >= 0),
    CHECK (rejected_processes >= 0)
);


CREATE TABLE IF NOT EXISTS process_state_history (
    history_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    run_id BIGINT NOT NULL
        REFERENCES simulation_runs(run_id)
        ON DELETE CASCADE,

    pid INT NOT NULL,

    previous_state VARCHAR(20),
    new_state VARCHAR(20) NOT NULL
        CHECK (new_state IN (
            'NEW',
            'WAITING',
            'READY',
            'RUNNING',
            'COMPLETED',
            'REJECTED'
        )),

    simulation_time INT NOT NULL
        CHECK (simulation_time >= 0),

    changed_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,

    details TEXT
);

COMMIT;