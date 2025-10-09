#include "scheduling_algo.h"

static int last_scheduled_index = -1;

// First Come First Serve Algorithm --> Based on arrival time
int scissos_schedule_fcfs(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ == NULL || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    fprintf(stdout, "[SCHEDULER: FCFS] Selecting first process in queue\n");
    return readyQ[0];
}

// Shortest Job First Algorithm --> Based on remaining instructions left
int scissos_schedule_sjf(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ == NULL || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    int selected_pid = readyQ[0];

    // Validate first PID
    if (selected_pid < 1 || selected_pid > MAXPROC || _proctable[selected_pid - 1] == NULL)
    {
        fprintf(stderr, "Error: Invalid PID %d in ready queue\n", selected_pid);
        return EMPTY;
    }

    int remaining = _proctable[selected_pid - 1]->size - _proctable[selected_pid - 1]->pc;

    // Find process with shortest remaining time
    for (int i = 1; i < qsize && readyQ[i] != EMPTY; i++)
    {
        int pid = readyQ[i];

        // Validate PID before accessing process table
        if (pid < 1 || pid > MAXPROC)
        {
            fprintf(stderr, "Error: Invalid PID %d in ready queue at index %d\n", pid, i);
            continue;
        }

        ScisSosPCB *pcb = _proctable[pid - 1];
        if (pcb == NULL)
        {
            fprintf(stderr, "Error: NULL PCB for PID %d\n", pid);
            continue;
        }

        int rem = pcb->size - pcb->pc;

        if (rem < remaining)
        {
            remaining = rem;
            selected_pid = pid;
        }
    }

    fprintf(stdout, "[SCHEDULER: SJF] Selected process %d (remaining=%d instructions)\n",
            selected_pid, remaining);

    return selected_pid;
}

// Priority Algorithm --> Based on process priority
int scissos_schedule_priority(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ == NULL || readyQ[0] == EMPTY)
    {
        return EMPTY;
    }

    int selected_pid = readyQ[0];

    // Validate first PID
    if (selected_pid < 1 || selected_pid > MAXPROC || _proctable[selected_pid - 1] == NULL)
    {
        fprintf(stderr, "Error: Invalid PID %d in ready queue\n", selected_pid);
        return EMPTY;
    }

    int highest_priority = _proctable[selected_pid - 1]->priority_value;

    // Find process with lowest priority value (highest priority)
    for (int i = 1; i < qsize && readyQ[i] != EMPTY; i++)
    {
        int pid = readyQ[i];

        // Validate PID before accessing process table
        if (pid < 1 || pid > MAXPROC)
        {
            fprintf(stderr, "Error: Invalid PID %d in ready queue at index %d\n", pid, i);
            continue;
        }

        ScisSosPCB *pcb = _proctable[pid - 1];
        if (pcb == NULL)
        {
            fprintf(stderr, "Error: NULL PCB for PID %d\n", pid);
            continue;
        }

        int priority = pcb->priority_value;

        if (priority < highest_priority)
        {
            highest_priority = priority;
            selected_pid = pid;
        }
    }

    fprintf(stdout, "[SCHEDULER: PRIORITY] Selected process %d (priority=%d)\n",
            selected_pid, highest_priority);

    return selected_pid;
}

// Round Robin Algorithm --> Based on time slice
int scissos_schedule_rr(int *readyQ, int qsize)
{
    if (qsize <= 0 || readyQ == NULL)
    {
        return EMPTY;
    }

    // Reset if last_scheduled_index is out of bounds
    if (last_scheduled_index >= qsize || last_scheduled_index < -1)
    {
        last_scheduled_index = -1;
    }

    int start_index = (last_scheduled_index + 1) % qsize;
    int selected_pid = EMPTY;

    // Loop through ready queue circularly to find the next valid process
    for (int i = 0; i < qsize; i++)
    {
        int index = (start_index + i) % qsize;

        if (readyQ[index] != EMPTY)
        {
            int pid = readyQ[index];

            // Validate PID
            if (pid >= 1 && pid <= MAXPROC && _proctable[pid - 1] != NULL)
            {
                selected_pid = pid;
                last_scheduled_index = index; // Update last scheduled index
                fprintf(stdout, "[SCHEDULER: ROUND ROBIN] Selected process %d (position %d in queue)\n",
                        selected_pid, index);
                return selected_pid;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid PID %d at index %d in ready queue\n", pid, index);
            }
        }
    }

    // No valid process found - reset index
    last_scheduled_index = -1;
    return EMPTY;
}