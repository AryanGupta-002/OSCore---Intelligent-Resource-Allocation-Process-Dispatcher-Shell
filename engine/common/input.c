
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"

#define MAX_PROCESSES 1000
#define MAX_LINE_LENGTH 256

int load_processes(
    const char *filename,
    Process processes[],
    int max_processes
) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open workload file.\n");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
    int line_number = 0;
    int has_line = 1;

    /* Accept a header row, but preserve a numeric first data row. */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        printf("Error: CSV file is empty.\n");
        return -1;
    }

    char *first_nonspace = line;
    while (*first_nonspace == ' ' || *first_nonspace == '\t') {
        first_nonspace++;
    }
    if ((*first_nonspace < '0' || *first_nonspace > '9') &&
        *first_nonspace != '+' && *first_nonspace != '-') {
        line_number = 1;
        has_line = fgets(line, sizeof(line), file) != NULL;
    }

    while (has_line) {
        line_number++;
        /* Ignore blank lines */
        if (line[0] == '\n' || line[0] == '\r') {
            has_line = fgets(line, sizeof(line), file) != NULL;
            continue;
        }

        if (count >= max_processes || count >= MAX_PROCESSES) {
            printf("Error: Too many processes in CSV.\n");
            fclose(file);
            return -1;
        }

        int values[8];
        int parsed = 0;
        char *token = strtok(line, ",");
        int valid = 1;

        while (token != NULL && parsed < 8) {
            char *end;
            errno = 0;
            long value = strtol(token, &end, 10);

            while (*end == ' ' || *end == '\t' ||
                   *end == '\r' || *end == '\n') {
                end++;
            }

            if (errno == ERANGE ||
                end == token ||
                *end != '\0' ||
                value < INT_MIN ||
                value > INT_MAX) {
                valid = 0;
                break;
            }

            values[parsed++] = (int)value;
            token = strtok(NULL, ",");
        }

        if (!valid || parsed != 8 || token != NULL) {
            printf("Error: Invalid CSV row at line %d.\n",
                   line_number);
            fclose(file);
            return -1;
        }

        if (values[0] < 0 ||
            values[1] < 0 || values[1] > 3 ||
            values[2] <= 0 ||
            values[3] <= 0 ||
            values[4] < 0 ||
            values[5] < 0 ||
            values[6] < 0 ||
            values[7] < 0) {
            printf("Error: Invalid values at line %d.\n",
                   line_number);
            fclose(file);
            return -1;
        }

        Process *p = &processes[count];

        initialize_process(p);

        p->pid = count + 1;
        snprintf(p->label, sizeof(p->label), "P%d", p->pid);

        p->arrival_time = values[0];
        p->priority = values[1];
        p->burst_time = values[2];
        p->remaining_time = values[2];

        p->memory_required_mb = values[3];
        p->printers_required = values[4];
        p->scanners_required = values[5];
        p->modems_required = values[6];
        p->cds_required = values[7];

        p->queue_level = p->priority;

        count++;
        has_line = fgets(line, sizeof(line), file) != NULL;
    }

    fclose(file);

    if (count == 0) {
        printf("Error: CSV contains no jobs.\n");
        return -1;
    }

    return count;
}
