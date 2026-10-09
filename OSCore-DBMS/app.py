import os
import csv
import io
from flask import Flask, render_template, request
import psycopg

app = Flask(__name__)


def get_connection():
    return psycopg.connect(
        host="localhost",
        port=5432,
        dbname="OScore",
        user="postgres",
        password=os.getenv("DB_PASSWORD")
    )


@app.route("/", methods=["GET", "POST"])
def index():

    if request.method == "POST":

        # Manual job entry
        if request.form.get("entry_type") == "manual":

            data = (
                request.form["job_name"],
                int(request.form["arrival_time"]),
                int(request.form["priority"]),
                int(request.form["cpu_time"]),
                int(request.form["memory_required"]),
                int(request.form.get("printer_required", 0)),
                int(request.form.get("scanner_required", 0)),
                int(request.form.get("modem_required", 0)),
                int(request.form.get("cd_required", 0))
            )

            with get_connection() as conn:
                with conn.cursor() as cur:
                    cur.execute("""
                        INSERT INTO jobs
                        (job_name, arrival_time, priority, cpu_time,
                         memory_required, printer_required,
                         scanner_required, modem_required, cd_required)
                        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                    """, data)

            return "Job submitted successfully!"

        # CSV file upload
        elif request.form.get("entry_type") == "file":

            uploaded_file = request.files.get("file")

            if not uploaded_file or not uploaded_file.filename:
                return "Please select a CSV file."

            if not uploaded_file.filename.lower().endswith(".csv"):
                return "Only CSV files are supported."

            try:
                content = uploaded_file.stream.read().decode("utf-8-sig")
                reader = csv.DictReader(io.StringIO(content))

                required_columns = {
                    "job_name", "arrival_time", "priority", "cpu_time",
                    "memory_required", "printer_required",
                    "scanner_required", "modem_required", "cd_required"
                }

                if not reader.fieldnames or not required_columns.issubset(
                    set(reader.fieldnames)
                ):
                    return "CSV is missing required columns."

                rows = []

                for row in reader:
                    rows.append((
                        row["job_name"],
                        int(row["arrival_time"]),
                        int(row["priority"]),
                        int(row["cpu_time"]),
                        int(row["memory_required"]),
                        int(row["printer_required"] or 0),
                        int(row["scanner_required"] or 0),
                        int(row["modem_required"] or 0),
                        int(row["cd_required"] or 0)
                    ))

                if not rows:
                    return "The CSV file contains no job records."

                with get_connection() as conn:
                    with conn.cursor() as cur:
                        cur.executemany("""
                            INSERT INTO jobs
                            (job_name, arrival_time, priority, cpu_time,
                             memory_required, printer_required,
                             scanner_required, modem_required, cd_required)
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                        """, rows)

                return f"Successfully imported {len(rows)} jobs!"

            except (ValueError, UnicodeDecodeError, csv.Error):
                return "Invalid CSV data. Check the file format and values."

    return render_template("index.html")


if __name__ == "__main__":
    app.run(debug=True)
