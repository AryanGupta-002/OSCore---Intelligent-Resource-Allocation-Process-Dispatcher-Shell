import os
from flask import Flask, render_template, request
import psycopg

app = Flask(__name__)

@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":

        data = (
            request.form["job_name"],
            request.form["arrival_time"],
            request.form["priority"],
            request.form["cpu_time"],
            request.form["memory_required"],
            request.form["printer_required"],
            request.form["scanner_required"],
            request.form["modem_required"],
            request.form["cd_required"]
        )

        with psycopg.connect(
            host="localhost",
            port=5432,
            dbname="OScore",
            user="postgres",
            password=os.getenv("DB_PASSWORD")
        ) as conn:

            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO jobs
                    (job_name, arrival_time, priority, cpu_time,
                     memory_required, printer_required, scanner_required,
                     modem_required, cd_required)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                """, data)

            conn.commit()

        return "Job submitted successfully!"

    return render_template("index.html")


if __name__ == "__main__":
    app.run(debug=True)