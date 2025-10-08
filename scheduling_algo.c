#include "ScisSos.h"

static int last_scheduled_index = -1;

// First Come First Serve Algorithm --> Based on arrival time
int scissos_schedule_fcfs(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    printf("[SCHEDULER: FCFS] Selecting first process in queue\n");
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
int scissos_schedule(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    // If this is the first scheduling or last process is not in queue
    if (last_scheduled_index == -1)
    {
        last_scheduled_index = 0;
        printf("[SCHEDULER: ROUND ROBIN] Starting from first process\n");
        return readyQ[0];
    }

    // Find the previously scheduled process in current ready queue
    int found_index = -1;
    for (int i = 0; i < qsize && readyQ[i] != EMPTY; i++)
    {
        if (i == last_scheduled_index && i < qsize)
        {
            found_index = i;
            break;
        }
    }

    // Move to next process in circular fashion
    int next_index = (last_scheduled_index + 1) % qsize;

    // Handle case where queue size changed
    if (next_index >= qsize || readyQ[next_index] == EMPTY)
    {
        next_index = 0;
    }

    last_scheduled_index = next_index;
    int selected_pid = readyQ[next_index];

    printf("[SCHEDULER: ROUND ROBIN] Selected process %d (position %d in queue)\n",
           selected_pid, next_index);

    return selected_pid;
}