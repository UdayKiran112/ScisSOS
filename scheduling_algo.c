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
    if (qsize <= 0 || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    // If this is the first scheduling
    if (last_scheduled_index == -1)
    {
        last_scheduled_index = 0;
        printf("[SCHEDULER: ROUND ROBIN] Starting from first process (PID: %d)\n", readyQ[0]);
        return readyQ[0];
    }

    // Find the position of the last scheduled process in current ready queue
    int found_position = -1;
    int last_scheduled_pid = readyQ[last_scheduled_index];

    for (int i = 0; i < qsize && readyQ[i] != EMPTY; i++)
    {
        if (readyQ[i] == last_scheduled_pid)
        {
            found_position = i;
            break;
        }
    }

    int next_index;

    // If last scheduled process is still in ready queue
    if (found_position != -1)
    {
        // Move to next process in circular fashion
        next_index = (found_position + 1) % qsize;

        // If we've wrapped around or hit an empty slot, go back to start
        if (readyQ[next_index] == EMPTY)
        {
            next_index = 0;
        }
    }
    else
    {
        // Last scheduled process is no longer in queue (blocked/completed)
        // Start from the beginning or continue from last index
        next_index = 0;
    }

    last_scheduled_index = next_index;
    int selected_pid = readyQ[next_index];

    printf("[SCHEDULER: ROUND ROBIN] Selected process %d (position %d in queue)\n",
           selected_pid, next_index);

    return selected_pid;
}