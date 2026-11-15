import csv
import os
import re
import subprocess
from pathlib import Path

import psycopg
from flask import Flask, jsonify, request, send_from_directory
from werkzeug.utils import secure_filename

BASE_DIR = Path(__file__).resolve().parent.parent
FRONTEND_DIR = BASE_DIR / "frontend"
DATA_DIR = BASE_DIR / "data"
ENGINE = BASE_DIR / "engine" / "host_engine"
UPLOAD_DIR = BASE_DIR / "backend" / "uploads"

app = Flask(
    __name__,
    static_folder=str(FRONTEND_DIR),
    static_url_path="",
)

UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
app.config["MAX_CONTENT_LENGTH"] = 5 * 1024 * 1024

ALGORITHMS = {"host", "fcfs", "sjf", "priority", "rr", "srtf", "mlfq"}
CSV_COLUMNS = [
    "arrival_time",
    "priority",
    "burst_time",
    "memory_required",
    "printers_required",
    "scanners_required",
    "modems_required",
    "ports_required",
]


def get_db_connection():
    password = os.getenv("DB_PASSWORD")
    if not password:
        raise RuntimeError("DB_PASSWORD environment variable is required")

    return psycopg.connect(
        host=os.getenv("DB_HOST", "localhost"),
        port=os.getenv("DB_PORT", "5432"),
        dbname=os.getenv("DB_NAME", "host_db"),
        user=os.getenv("DB_USER", "host"),
        password=password,
        connect_timeout=5,
    )


def validate_csv(file_path):
    with open(file_path, newline="", encoding="utf-8-sig") as csv_file:
        reader = csv.reader(csv_file)
        rows = list(reader)

    if not rows:
        raise ValueError("CSV file is empty.")

    # Accept either a header row or headerless numeric CSV.
    header = [cell.strip().lower() for cell in rows[0]]

    if header in (
        ["arrival", "priority", "cpu", "memory", "printers", "scanners", "modems", "cds"],
        CSV_COLUMNS,
    ):
        rows = rows[1:]
        

    if not rows:
        raise ValueError("CSV contains no process records.")
    if len(rows) > 1000:
        raise ValueError("CSV contains more than 1000 process records.")

    for line_number, row in enumerate(rows, start=1):
        if len(row) != 8:
            raise ValueError(
                f"Row {line_number} must contain exactly 8 values."
            )

        try:
            values = [int(value.strip()) for value in row]
        except ValueError:
            raise ValueError(
                f"Row {line_number} contains a non-integer value."
            )

        if any(value > 2147483647 for value in values):
            raise ValueError(
                f"Row {line_number} contains a value outside the supported range."
            )

        arrival, priority, burst, memory, printers, scanners, modems, cds = values

        if arrival < 0 or burst <= 0 or memory <= 0:
            raise ValueError(
                f"Row {line_number}: arrival must be >= 0, burst and memory must be > 0."
            )

        if priority not in (0, 1, 2, 3):
            raise ValueError(
                f"Row {line_number}: priority must be between 0 and 3."
            )

        if any(value < 0 for value in values[3:]):
            raise ValueError(
                f"Row {line_number}: memory and device requirements cannot be negative."
            )
    return len(rows)


@app.get("/")
def frontend():
    return send_from_directory(FRONTEND_DIR, "index.html")


@app.get("/api/health")
def health():
    return jsonify({
        "status": "ok",
        "engine_exists": ENGINE.is_file(),
    })


@app.get("/api/workloads")
def workloads():
    available = []

    for path in sorted(DATA_DIR.glob("*.csv")):
        available.append({
            "id": path.name,
            "filename": path.name,
            "name": path.name,
            "csv_text": path.read_text(encoding="utf-8-sig"),
        })

    return jsonify({"workloads": available})


@app.get("/api/algorithms")
def algorithms():
    return jsonify({
        "algorithms": [
            {"id": "host", "name": "HOST Dispatcher"},
            {"id": "fcfs", "name": "First Come First Served"},
            {"id": "sjf", "name": "Shortest Job First"},
            {"id": "priority", "name": "Priority Scheduling"},
            {"id": "rr", "name": "Round Robin"},
            {"id": "srtf", "name": "Shortest Remaining Time First"},
            {"id": "mlfq", "name": "Multilevel Feedback Queue"},
        ]
    })


@app.post("/api/run")
def run_simulation():
    if "file" not in request.files:
        return jsonify({"error": "Upload a CSV using the 'file' field."}), 400

    uploaded_file = request.files["file"]
    algorithm = request.form.get("algorithm", "host").strip().lower()

    if algorithm not in ALGORITHMS:
        return jsonify({"error": "Unsupported scheduling algorithm."}), 400

    if not uploaded_file.filename:
        return jsonify({"error": "Please select a CSV file."}), 400

    if Path(uploaded_file.filename).suffix.lower() != ".csv":
        return jsonify({"error": "Only CSV files are accepted."}), 400

    try:
        time_quantum = int(request.form.get("time_quantum", "0"))
    except ValueError:
        return jsonify({"error": "Time quantum must be a positive integer."}), 400

    if algorithm == "rr" and time_quantum <= 0:
        return jsonify({"error": "Round Robin requires a positive time quantum."}), 400

    if not 0 <= time_quantum <= 2147483647:
        return jsonify({"error": "Time quantum is outside the supported range."}), 400

    custom_machine = request.form.get("machine_profile") == "custom"
    if custom_machine and algorithm != "host":
        return jsonify({
            "error": "Custom machine capacities can only be used with HOST."
        }), 400

    machine_config = None
    if custom_machine:
        try:
            machine_config = {
                "total_memory_mb": int(request.form.get("total_memory_mb", "")),
                "printers": int(request.form.get("printers", "")),
                "scanners": int(request.form.get("scanners", "")),
                "modems": int(request.form.get("modems", "")),
                "cds": int(request.form.get("cds", "")),
            }
        except ValueError:
            return jsonify({
                "error": "Custom machine capacities must be whole numbers."
            }), 400

        if machine_config["total_memory_mb"] < 2:
            return jsonify({
                "error": "Custom machine memory must be at least 2 MB."
            }), 400

        if any(value < 0 or value > 2147483647
               for value in machine_config.values()):
            return jsonify({
                "error": "Custom machine capacities are outside the supported range."
            }), 400

    if not ENGINE.is_file():
        return jsonify({"error": "C engine executable was not found."}), 500

    safe_name = secure_filename(uploaded_file.filename)
    if not safe_name:
        return jsonify({"error": "Invalid filename."}), 400

    # Use a unique filename so simultaneous uploads do not overwrite each other.
    import uuid
    csv_path = UPLOAD_DIR / f"{uuid.uuid4().hex}_{safe_name}"

    try:
        uploaded_file.save(csv_path)
        process_count = validate_csv(csv_path)

        command = [
            str(ENGINE),
            str(csv_path),
            algorithm,
            str(time_quantum),
            safe_name,
        ]
        if machine_config:
            command.extend(str(value) for value in (
                machine_config["total_memory_mb"],
                machine_config["printers"],
                machine_config["scanners"],
                machine_config["modems"],
                machine_config["cds"],
            ))

        result = subprocess.run(
            command,
            cwd=BASE_DIR,
            capture_output=True,
            text=True,
            timeout=120,
        )

        if result.returncode != 0:
            return jsonify({
                "error": "Simulation failed.",
                "details": result.stderr[-4000:],
                "output": result.stdout[-4000:],
            }), 500

        run_match = re.search(r"OSCORE_RUN_ID=(\d+)", result.stdout)
        if run_match is None:
            app.logger.error("C engine completed without returning a simulation run ID.")
            return jsonify({
                "error": "Simulation completed without a run ID.",
                "output": result.stdout[-4000:],
            }), 500

        return jsonify({
            "message": "Simulation completed.",
            "algorithm": algorithm,
            "display_algorithm": (
                "Custom Machine" if custom_machine else algorithm.upper()
            ),
            "machine_profile": "custom" if custom_machine else "standard",
            "run_id": int(run_match.group(1)),
            "process_count": process_count,
            "output": result.stdout,
            "stderr": result.stderr,
        })

    except ValueError as error:
        return jsonify({"error": str(error)}), 400
    except subprocess.TimeoutExpired:
        return jsonify({"error": "Simulation exceeded the 120-second limit."}), 504
    except psycopg.Error:
        app.logger.exception("Database error during simulation.")
        return jsonify({"error": "Database connection or query failed."}), 500
    except OSError:
        app.logger.exception("Could not execute the C engine.")
        return jsonify({"error": "Could not start the C engine."}), 500
    finally:
        csv_path.unlink(missing_ok=True)


@app.get("/api/runs")
def list_runs():
    try:
        with get_db_connection() as conn:
            rows = conn.execute("""
                SELECT r.run_id, r.workload_id, r.algorithm, r.run_status,
                       r.total_execution_time, r.cpu_utilization,
                       r.completed_processes, r.rejected_processes,
                       r.started_at, r.finished_at, w.workload_name,
                       w.source_filename
                FROM simulation_runs AS r
                JOIN workloads AS w ON w.workload_id = r.workload_id
                ORDER BY r.run_id DESC
                LIMIT 10
            """).fetchall()

        return jsonify({
            "runs": [
                {
                    "run_id": row[0],
                    "workload_id": row[1],
                    "algorithm": row[2],
                    "status": row[3],
                    "total_execution_time": row[4],
                    "cpu_utilization": float(row[5]) if row[5] is not None else None,
                    "completed_processes": row[6],
                    "rejected_processes": row[7],
                    "started_at": row[8].isoformat() if row[8] else None,
                    "finished_at": row[9].isoformat() if row[9] else None,
                    "workload_name": row[10],
                    "source_filename": row[11],
                }
                for row in rows
            ]
        })
    except psycopg.Error:
        app.logger.exception("Could not fetch simulation runs.")
        return jsonify({"error": "Could not retrieve simulation runs."}), 500


@app.delete("/api/runs")
def delete_runs():
    try:
        with get_db_connection() as conn:
            deleted = conn.execute(
                "DELETE FROM simulation_runs"
            ).rowcount

        return jsonify({
            "message": "Simulation history deleted.",
            "deleted_runs": deleted,
        })
    except psycopg.Error:
        app.logger.exception("Could not delete simulation history.")
        return jsonify({"error": "Could not delete simulation history."}), 500


@app.get("/api/runs/<int:run_id>/results")
def run_results(run_id):
    try:
        with get_db_connection() as conn:
            run = conn.execute("""
                SELECT run_id, workload_id, algorithm, run_status,
                       total_execution_time, cpu_utilization,
                       completed_processes, rejected_processes
                FROM simulation_runs
                WHERE run_id = %s
            """, (run_id,)).fetchone()

            if run is None:
                return jsonify({"error": "Simulation run not found."}), 404

            processes = conn.execute("""
                SELECT pid, arrival_time, burst_time, completion_time,
                       turnaround_time, waiting_time, response_time, final_state
                FROM process_results
                WHERE run_id = %s
                ORDER BY pid
            """, (run_id,)).fetchall()

            metrics = conn.execute("""
                SELECT average_turnaround_time, average_waiting_time,
                       average_response_time, cpu_utilization, throughput,
                       total_execution_time, completed_processes,
                       rejected_processes
                FROM performance_metrics
                WHERE run_id = %s
            """, (run_id,)).fetchone()

            events = conn.execute("""
                SELECT pid, event_type, start_time, end_time, priority, details
                FROM execution_events
                WHERE run_id = %s
                ORDER BY event_id
            """, (run_id,)).fetchall()

        return jsonify({
            "run": {
                "run_id": run[0],
                "workload_id": run[1],
                "algorithm": run[2],
                "status": run[3],
                "total_execution_time": run[4],
                "cpu_utilization": float(run[5]) if run[5] is not None else None,
                "completed_processes": run[6],
                "rejected_processes": run[7],
            },
            "processes": [
                {
                    "pid": p[0], "arrival_time": p[1], "burst_time": p[2],
                    "completion_time": p[3], "turnaround_time": p[4],
                    "waiting_time": p[5], "response_time": p[6],
                    "final_state": p[7],
                }
                for p in processes
            ],
            "metrics": (
                {
                    "average_turnaround_time": metrics[0],
                    "average_waiting_time": metrics[1],
                    "average_response_time": metrics[2],
                    "cpu_utilization": float(metrics[3]) if metrics[3] is not None else None,
                    "throughput": float(metrics[4]) if metrics[4] is not None else None,
                    "total_execution_time": metrics[5],
                    "completed_processes": metrics[6],
                    "rejected_processes": metrics[7],
                }
                if metrics else None
            ),
            "events": [
                {
                    "pid": e[0], "event_type": e[1], "start_time": e[2],
                    "end_time": e[3], "priority": e[4], "details": e[5],
                }
                for e in events
            ],
        })

    except psycopg.Error:
        app.logger.exception("Could not fetch run results.")
        return jsonify({"error": "Could not retrieve run results."}), 500


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=False)
