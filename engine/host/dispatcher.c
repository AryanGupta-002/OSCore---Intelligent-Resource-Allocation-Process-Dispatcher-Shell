#include <stddef.h>
#include "../include/process.h"
#include "host.h"

/*
 * Select the next READY process.
 *
 * Priority 0: Real-time
 * Priority 1: Highest user priority
 * Priority 2: Medium user priority
 * Priority 3: Lowest user priority
 *
 * For equal priorities:
 * 1. Earlier arrival time wins.
 * 2. If arrival times match, lower PID wins.
 */
int select_next_process(
    Process processes[],
    int process_count
)
{
    int selected = -1;

    for (int i = 0; i < process_count; i++) {

        if (processes[i].state != PROCESS_READY) {
            continue;
        }

        if (selected == -1) {
            selected = i;
            continue;
        }

        if (processes[i].priority < processes[selected].priority) {
            selected = i;
        }
        else if (
            processes[i].priority == processes[selected].priority &&
            processes[i].arrival_time < processes[selected].arrival_time
        ) {
            selected = i;
        }
        else if (
            processes[i].priority == processes[selected].priority &&
            processes[i].arrival_time == processes[selected].arrival_time &&
            processes[i].pid < processes[selected].pid
        ) {
            selected = i;
        }
    }

    return selected;
}

/*
 * Demote a user process by one priority level.
 * Priority 3 is the lowest level, so it cannot
 * be demoted further.
 *
 * Real-time processes are never demoted.
 */
void demote_user_process(Process *p)
{
    if (p == NULL) {
        return;
    }

    if (p->priority >= PRIORITY_LEVEL_1 &&
        p->priority < PRIORITY_LEVEL_3) {

        p->priority++;
        p->queue_level = p->priority;
    }
}
