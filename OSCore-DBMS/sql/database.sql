-- OSCore DBMS
-- PostgreSQL database setup

-- 1. Users

CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO users (username, email)
VALUES
('Admin', 'admin@oscore.com'),
('Student', 'student@oscore.com'),
('User3', 'user3@oscore.com'),
('User4', 'user4@oscore.com'),
('User5', 'user5@oscore.com'),
('User6', 'user6@oscore.com'),
('User7', 'user7@oscore.com'),
('User8', 'user8@oscore.com'),
('User9', 'user9@oscore.com'),
('User10', 'user10@oscore.com'),
('User11', 'user11@oscore.com'),
('User12', 'user12@oscore.com'),
('User13', 'user13@oscore.com'),
('User14', 'user14@oscore.com'),
('User15', 'user15@oscore.com');


-- 2. Jobs

CREATE TABLE jobs (
    job_id SERIAL PRIMARY KEY,
    job_name VARCHAR(100) NOT NULL,
    arrival_time INT NOT NULL,
    priority INT NOT NULL,
    cpu_time INT NOT NULL,
    memory_required INT NOT NULL,
    printer_required INT DEFAULT 0,
    scanner_required INT DEFAULT 0,
    modem_required INT DEFAULT 0,
    cd_required INT DEFAULT 0,
    status VARCHAR(20) DEFAULT 'NEW',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INT,

    CONSTRAINT fk_job_user
        FOREIGN KEY (user_id)
        REFERENCES users(user_id)
);

INSERT INTO jobs
(job_name, arrival_time, priority, cpu_time, memory_required,
 printer_required, scanner_required, modem_required, cd_required, user_id)
VALUES
('Compiler', 0, 2, 10, 256, 1, 0, 0, 0, 1),
('Chrome', 2, 3, 20, 512, 0, 0, 1, 0, 2),
('Database', 4, 1, 15, 384, 0, 1, 0, 0, 3),
('Editor', 6, 4, 8, 128, 0, 0, 0, 0, 4),
('TestJob', 10, 1, 5, 256, 0, 0, 0, 0, 5),
('job2', 2, 1, 8, 512, 0, 0, 0, 0, 6),
('Terminal', 8, 2, 12, 256, 0, 0, 0, 0, 7),
('Browser', 10, 3, 18, 512, 0, 0, 1, 0, 8),
('Compiler2', 12, 1, 14, 384, 1, 0, 0, 0, 9),
('MusicPlayer', 14, 4, 6, 128, 0, 0, 0, 0, 10),
('FileManager', 16, 2, 10, 256, 0, 1, 0, 0, 11),
('IDE', 18, 1, 25, 512, 0, 0, 0, 1, 12),
('Calculator', 20, 5, 5, 128, 0, 0, 0, 0, 13),
('MediaPlayer', 22, 3, 16, 384, 0, 0, 1, 0, 14),
('Backup', 24, 2, 20, 512, 0, 0, 0, 1, 15);


-- 3. Processes

CREATE TABLE processes (
    process_id SERIAL PRIMARY KEY,
    job_id INT NOT NULL,
    process_state VARCHAR(20) NOT NULL DEFAULT 'NEW',
    pid INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_process_job
        FOREIGN KEY (job_id)
        REFERENCES jobs(job_id)
);

INSERT INTO processes
(job_id, process_state, pid)
VALUES
(1, 'NEW', 1001),
(2, 'READY', 1002),
(3, 'RUNNING', 1003),
(4, 'TERMINATED', 1004),
(5, 'READY', 1005),
(6, 'RUNNING', 1006),
(7, 'NEW', 1007),
(8, 'READY', 1008),
(9, 'RUNNING', 1009),
(10, 'NEW', 1010),
(11, 'READY', 1011),
(12, 'RUNNING', 1012),
(13, 'NEW', 1013),
(14, 'READY', 1014),
(15, 'TERMINATED', 1015);


-- 4. Resource Requests

CREATE TABLE resource_requests (
    request_id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    printer_required INT DEFAULT 0,
    scanner_required INT DEFAULT 0,
    modem_required INT DEFAULT 0,
    cd_required INT DEFAULT 0,

    CONSTRAINT fk_request_process
        FOREIGN KEY (process_id)
        REFERENCES processes(process_id)
);

INSERT INTO resource_requests
(process_id, printer_required, scanner_required, modem_required, cd_required)
VALUES
(1, 1, 0, 0, 0),
(2, 0, 0, 1, 0),
(3, 0, 1, 0, 0),
(4, 0, 0, 0, 0),
(5, 0, 0, 0, 0),
(6, 1, 0, 0, 0),
(7, 0, 1, 0, 0),
(8, 0, 0, 1, 0),
(9, 0, 0, 0, 1),
(10, 0, 0, 0, 0),
(11, 1, 0, 0, 0),
(12, 0, 1, 0, 0),
(13, 0, 0, 1, 0),
(14, 0, 0, 0, 1),
(15, 0, 0, 0, 0);


-- 5. Resource Allocations

CREATE TABLE resource_allocations (
    allocation_id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    printer_allocated INT DEFAULT 0,
    scanner_allocated INT DEFAULT 0,
    modem_allocated INT DEFAULT 0,
    cd_allocated INT DEFAULT 0,

    CONSTRAINT fk_allocation_process
        FOREIGN KEY (process_id)
        REFERENCES processes(process_id)
);

INSERT INTO resource_allocations
(process_id, printer_allocated, scanner_allocated, modem_allocated, cd_allocated)
VALUES
(1, 1, 0, 0, 0),
(2, 0, 0, 1, 0),
(3, 0, 1, 0, 0),
(4, 0, 0, 0, 0),
(5, 0, 0, 0, 0),
(6, 1, 0, 0, 0),
(7, 0, 1, 0, 0),
(8, 0, 0, 1, 0),
(9, 0, 0, 0, 1),
(10, 0, 0, 0, 0),
(11, 1, 0, 0, 0),
(12, 0, 1, 0, 0),
(13, 0, 0, 1, 0),
(14, 0, 0, 0, 1),
(15, 0, 0, 0, 0);


-- 6. Process Events

CREATE TABLE process_events (
    event_id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    event_type VARCHAR(30) NOT NULL,
    event_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_event_process
        FOREIGN KEY (process_id)
        REFERENCES processes(process_id)
);

INSERT INTO process_events (process_id, event_type)
VALUES
(1, 'NEW'),
(1, 'READY'),
(1, 'RUNNING'),
(1, 'TERMINATED'),
(2, 'NEW'),
(2, 'READY'),
(3, 'NEW'),
(3, 'RUNNING'),
(4, 'NEW'),
(4, 'TERMINATED'),
(5, 'NEW'),
(6, 'NEW'),
(7, 'NEW'),
(8, 'NEW'),
(9, 'NEW'),
(10, 'NEW'),
(11, 'NEW'),
(12, 'NEW'),
(13, 'NEW'),
(14, 'NEW'),
(15, 'NEW');


-- 7. Execution Metrics

CREATE TABLE execution_metrics (
    metric_id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    waiting_time INT DEFAULT 0,
    turnaround_time INT DEFAULT 0,
    response_time INT DEFAULT 0,
    cpu_utilization DECIMAL(5,2) DEFAULT 0,
    throughput DECIMAL(10,2) DEFAULT 0,
    context_switches INT DEFAULT 0,
    preemptions INT DEFAULT 0,
    starvation INT DEFAULT 0,
    resource_waiting_time INT DEFAULT 0,

    CONSTRAINT fk_metric_process
        FOREIGN KEY (process_id)
        REFERENCES processes(process_id)
);

INSERT INTO execution_metrics
(process_id, waiting_time, turnaround_time, response_time,
 cpu_utilization, throughput, context_switches,
 preemptions, starvation, resource_waiting_time)
VALUES
(1, 5, 15, 2, 80.00, 0.25, 3, 1, 0, 2),
(2, 8, 20, 4, 75.00, 0.20, 4, 2, 0, 3),
(3, 3, 12, 1, 90.00, 0.30, 2, 1, 0, 1),
(4, 10, 18, 5, 70.00, 0.18, 5, 2, 1, 4),
(5, 6, 14, 2, 82.00, 0.24, 3, 1, 0, 2),
(6, 7, 16, 3, 85.00, 0.27, 4, 1, 0, 2),
(7, 4, 11, 1, 88.00, 0.31, 2, 1, 0, 1),
(8, 9, 19, 4, 76.00, 0.21, 5, 2, 0, 3),
(9, 5, 13, 2, 89.00, 0.29, 3, 1, 0, 1),
(10, 8, 17, 3, 80.00, 0.23, 4, 2, 0, 2),
(11, 3, 10, 1, 92.00, 0.34, 2, 1, 0, 1),
(12, 11, 22, 5, 74.00, 0.19, 6, 3, 1, 4),
(13, 6, 15, 2, 87.00, 0.28, 3, 1, 0, 2),
(14, 10, 20, 4, 79.00, 0.22, 5, 2, 0, 3),
(15, 7, 18, 3, 83.00, 0.26, 4, 2, 0, 2);


-- 8. Scheduling Runs

CREATE TABLE scheduling_runs (
    run_id SERIAL PRIMARY KEY,
    algorithm VARCHAR(30) NOT NULL,
    average_waiting_time DECIMAL(10,2),
    average_turnaround_time DECIMAL(10,2),
    average_response_time DECIMAL(10,2),
    cpu_utilization DECIMAL(5,2),
    throughput DECIMAL(10,2),
    context_switches INT DEFAULT 0,
    preemptions INT DEFAULT 0,
    run_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO scheduling_runs
(algorithm, average_waiting_time, average_turnaround_time,
 average_response_time, cpu_utilization, throughput,
 context_switches, preemptions)
VALUES
('FCFS', 18.40, 26.10, 12.50, 78.00, 0.25, 12, 2),
('SJF', 10.20, 18.70, 7.80, 84.00, 0.30, 15, 3),
('SRTF', 7.80, 15.30, 5.20, 88.00, 0.32, 42, 8),
('Round Robin', 9.10, 17.20, 6.10, 91.00, 0.35, 67, 12),
('Priority', 11.50, 19.40, 8.00, 86.00, 0.29, 29, 5),
('MLFQ', 7.20, 14.80, 4.90, 93.00, 0.36, 35, 7),
('FCFS-2', 17.20, 25.40, 11.80, 79.00, 0.26, 13, 3),
('SJF-2', 9.80, 17.90, 7.20, 85.00, 0.31, 14, 3),
('SRTF-2', 8.10, 15.80, 5.50, 87.00, 0.33, 40, 8),
('RR-2', 9.50, 17.60, 6.40, 90.00, 0.34, 65, 12),
('Priority-2', 11.10, 19.10, 7.70, 86.00, 0.30, 28, 5),
('MLFQ-2', 7.50, 15.10, 5.10, 94.00, 0.37, 36, 7),
('FCFS-3', 16.90, 24.80, 11.30, 80.00, 0.27, 12, 2),
('SJF-3', 10.00, 18.20, 7.50, 84.00, 0.30, 15, 3),
('RR-3', 9.00, 17.00, 6.00, 92.00, 0.36, 66, 11);