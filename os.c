#include "scheduling_algo.h"
#include "time.h"

int _currentPID = EMPTY;
ScisSosPCB *_proctable[MAXPROC] = {NULL};
int _readyQ[MAXPROC] = {EMPTY};
int _blockQ[MAXPROC] = {EMPTY};

// Initialise the OS
void scissos_initialise(void)
{
    fprintf(stdout, "=== Initialising ScisSOS ===\n");

    // Process table initialisation
    for (int i = 0; i < MAXPROC; i++)
    {
        _proctable[i] = NULL;
        _readyQ[i] = EMPTY;
        _blockQ[i] = EMPTY;
    }

    _currentPID = EMPTY;

    // Seed random number generator
    srand((unsigned int)time(NULL));

    fprintf(stdout, "Process table initialised\n");
    fprintf(stdout, "Ready and Block Queues initialised\n");
    fprintf(stdout, "=== ScisSOS Initialised ===\n\n");
}

// Update the ready and block queues
void scisos_update_queues(void)
{
    /* READY QUEUE UPDATION*/
    int q_index = 0;

    // clear ready queue
    for (int i = 0; i < MAXPROC; i++)
    {
        _readyQ[i] = EMPTY;
    }

    // check process table for ready processes
    for (int i = 0; i < MAXPROC; i++)
    {
        ScisSosPCB *pcb = _proctable[i];

        if (pcb != NULL && pcb->ps_state == PS_RDY)
        {
            _readyQ[q_index++] = pcb->pid;
        }
    }

    /* BLOCK QUEUE UPDATION*/
    int b_index = 0;

    // clear block queue
    for (int i = 0; i < MAXPROC; i++)
    {
        _blockQ[i] = EMPTY;
    }

    // check process table for blocked processes
    for (int i = 0; i < MAXPROC; i++)
    {
        ScisSosPCB *pcb = _proctable[i];

        if (pcb != NULL && pcb->ps_state == PS_BLK)
        {
            _blockQ[b_index++] = pcb->pid;
        }
    }
}

// count ready processes
int scissos_count_ready_processes(void)
{
    int count = 0;

    for (int i = 0; i < MAXPROC; i++)
    {
        if (_readyQ[i] != EMPTY)
        {
            count++;
        }
    }

    return count;
}

// check for active processes
int scisos_active_processes(void)
{
    for (int i = 0; i < MAXPROC; i++)
    {
        ScisSosPCB *pcb = _proctable[i];
        if (pcb != NULL)
        {
            int state = pcb->ps_state;

            if (state == PS_RDY || state == PS_RUN || state == PS_BLK ||
                state == PS_SRDY || state == PS_SBLK)
            {
                return 1; // Active process found
            }
        }
    }
    return 0; // No active processes
}

// Move blocked process to ready state
void scissos_unblock_process(void)
{
    for (int i = 0; i < MAXPROC; i++)
    {
        if (_blockQ[i] != EMPTY)
        {
            int pid = _blockQ[i];

            // Add bounds check
            if (pid < 1 || pid > MAXPROC)
            {
                fprintf(stderr, "Warning: Invalid PID %d in block queue\n", pid);
                continue;
            }

            ScisSosPCB *pcb = _proctable[pid - 1];

            if (pcb != NULL && pcb->ps_state == PS_BLK)
            {
                pcb->ps_state = PS_RDY;
                fprintf(stdout, "[UNBLOCKED] Process PID %d moved to READY state\n", pcb->pid);
            }
        }
    }
}

// Call the scheduler to manage processes
void scissos_call_scheduler(char *scheduler)
{
    fprintf(stdout, "\n=== SCHEDULER INVOKED ===\n");

    // unblock processes
    scissos_unblock_process();

    // update ready queue and block queue
    scisos_update_queues();

    int ready_count = scissos_count_ready_processes();

    fprintf(stdout, "Number of ready processes: %d\n", ready_count);

    // Print Ready queue
    if (ready_count > 0)
    {
        fprintf(stdout, "Ready Queue: [");

        int first = 1;
        for (int i = 0; i < MAXPROC && _readyQ[i] != EMPTY; i++)
        {
            if (!first)
            {
                fprintf(stdout, ", ");
            }
            fprintf(stdout, "%d", _readyQ[i]);
            first = 0;
        }
        fprintf(stdout, "]\n");
    }

    // check for active processes
    if (!scisos_active_processes())
    {
        fprintf(stdout, "No active processes --- Scheduler terminating\n");
        fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
        return;
    }

    // // if no ready processes --> terminate
    // if (ready_count == 0)
    // {
    //     fprintf(stdout, "No ready processes --- Scheduler terminating\n");
    //     fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
    //     return;
    // }

    // Change current running process to READY (if exists)
    if (_currentPID != EMPTY && _currentPID > 0 && _currentPID <= MAXPROC)
    {
        ScisSosPCB *current_pcb = _proctable[_currentPID - 1];
        if (current_pcb != NULL && current_pcb->ps_state == PS_RUN)
        {
            current_pcb->ps_state = PS_RDY;
        }
    }

    // call scheduling_algo
    int selected_pid = EMPTY;
    if (strcmp(scheduler, "fcfs") == 0)
    {
        selected_pid = scissos_schedule_fcfs(_readyQ, ready_count);
    }
    else if (strcmp(scheduler, "sjf") == 0)
    {
        selected_pid = scissos_schedule_sjf(_readyQ, ready_count);
    }
    else if (strcmp(scheduler, "rr") == 0)
    {
        selected_pid = scissos_schedule_rr(_readyQ, ready_count);
    }
    else if (strcmp(scheduler, "priority") == 0)
    {
        selected_pid = scissos_schedule_priority(_readyQ, ready_count);
    }
    else
    {
        fprintf(stderr, "Error: Unknown scheduler '%s'\n", scheduler);
        fprintf(stdout, "Available schedulers: fcfs, sjf, priority, rr\n");
        fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
        return;
    }

    // check for valid selected_pid
    if (selected_pid <= 0 || selected_pid > MAXPROC || _proctable[selected_pid - 1] == NULL)
    {
        fprintf(stdout, "Invalid process with PID %d selected for scheduling\n", selected_pid);
        fprintf(stdout, "Scheduler terminating\n");
        fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
        return;
    }

    fprintf(stdout, "\n[SCHEDULED] Process %d selected for execution\n", selected_pid);

    // update process state to running
    _proctable[selected_pid - 1]->ps_state = PS_RUN;
    _currentPID = selected_pid;

    fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");

    // Run the selected process
    scissos_proc_run(selected_pid, scheduler);
}