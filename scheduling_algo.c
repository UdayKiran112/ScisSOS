#include "scheduling_algo.h"

static int last_scheduled_index = -1;

// First Come First Serve Algorithm --> Based on arrival time
int scissos_schedule_fcfs(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    printf("[SCHEDULER: FCFS] Selecting first process in queue\n");
    if (readyQ[0] == EMPTY)
    {
        return EMPTY;
    }
    return readyQ[0];
}

// Shortest Job First Algorithm --> Based on remaining instructions left
int scissos_schedule_sjf(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    int selected_pid = readyQ[0];
    int remaining = _proctable[selected_pid]->size - _proctable[selected_pid]->pc;

    // Find process with shortest remaining time
    for (int i = 1; i < qsize && readyQ[i] != EMPTY; i++)
    {
        int pid = readyQ[i];
        int rem = _proctable[pid]->size - _proctable[pid]->pc;

        if (rem < remaining)
        {
            remaining = rem;
            selected_pid = pid;
        }
    }

    printf("[SCHEDULER: SJF] Selected process %d (remaining=%d instructions)\n",
           selected_pid, remaining);

    return selected_pid;
}

// Priority Algorithm --> Based on process priority
int scissos_schedule_priority(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    int selected_pid = readyQ[0];
    int highest_priority = _proctable[selected_pid]->priority_value;

    // Find process with lowest priority value (highest priority)
    for (int i = 1; i < qsize && readyQ[i] != EMPTY; i++)
    {
        int pid = readyQ[i];
        int priority = _proctable[pid]->priority_value;

        if (priority < highest_priority)
        {
            highest_priority = priority;
            selected_pid = pid;
        }
    }

    printf("[SCHEDULER: PRIORITY] Selected process %d (priority=%d)\n",
           selected_pid, highest_priority);

    return selected_pid;
}

// Round Robin Algorithm --> Based on time slice
int scissos_schedule_rr(int *readyQ, int qsize)
{
    if (qsize <= 0)
        return EMPTY;

    int start_index = (last_scheduled_index + 1) % qsize;
    int selected_pid = EMPTY;

    // Loop through ready queue circularly to find the next valid process
    for (int i = 0; i < qsize; i++)
    {
        int index = (start_index + i) % qsize;
        if (readyQ[index] != EMPTY)
        {
            selected_pid = readyQ[index];
            last_scheduled_index = index; // Update last scheduled index
            printf("[SCHEDULER: ROUND ROBIN] Selected process %d (position %d in queue)\n",
                   selected_pid, index);
            return selected_pid;
        }
    }

    // No valid process found
    return EMPTY;
}
