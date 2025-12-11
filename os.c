/* os.c - corrected version to match ScisSos.h and fix wait-time/linker issues */

#include "scheduling_algo.h"
#include "time.h"
#include "ScisSos.h"
#include <sys/time.h> /* for gettimeofday */
#include <string.h>   /* for strcmp */

/* Global OS data (from header) */
int _currentPID = EMPTY;
ScisSosPCB *_proctable[MAXPROC] = {NULL};
int _readyQ[MAXPROC] = {EMPTY};
int _blockQ[MAXPROC] = {EMPTY};

/* Local array to track when a PID entered READY last (microseconds).
   We keep this inside os.c to avoid modifying the header/PCB. */
static long _ready_enter_time[MAXPROC] = {0};

/* Helper: get current time in microseconds */
static long now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

/* Initialise the OS */
void scissos_initialise(void)
{
    fprintf(stdout, "=== Initialising ScisSOS ===\n");

    /* Process table initialisation */
    for (int i = 0; i < MAXPROC; i++)
    {
        _proctable[i] = NULL;
        _readyQ[i] = EMPTY;
        _blockQ[i] = EMPTY;
        _ready_enter_time[i] = 0;
    }

    _currentPID = EMPTY;

    /* Seed random number generator */
    srand((unsigned int)time(NULL));

    fprintf(stdout, "Process table initialised\n");
    fprintf(stdout, "Ready and Block Queues initialised\n");

    /* Initialize memory manager if enabled */
    if (_memory_manager_enabled)
    {
        memory_manager_init(NUMFRAMES);
    }

    fprintf(stdout, "=== ScisSOS Initialised ===\n\n");
}

/* Update the ready and block queues
   Also: if a PCB is READY and we haven't recorded when it entered READY,
   record the current time in _ready_enter_time. */
void scissos_update_queues(void)
{
    /* READY QUEUE UPDATION*/
    int q_index = 0;

    /* clear ready queue */
    for (int i = 0; i < MAXPROC; i++)
    {
        _readyQ[i] = EMPTY;
    }

    /* check process table for ready processes */
    for (int i = 0; i < MAXPROC; i++)
    {
        ScisSosPCB *pcb = _proctable[i];

        if (pcb != NULL && pcb->ps_state == PS_RDY)
        {
            /* guard PID bounds just in case */
            if (pcb->pid >= 1 && pcb->pid <= MAXPROC)
            {
                _readyQ[q_index++] = pcb->pid;

                /* If this process entered READY and we don't have a timestamp,
                   set the ready enter time now. */
                if (_ready_enter_time[pcb->pid - 1] == 0)
                {
                    _ready_enter_time[pcb->pid - 1] = now_us();
                }
            }
        }
    }

    /* BLOCK QUEUE UPDATION*/
    int b_index = 0;

    /* clear block queue */
    for (int i = 0; i < MAXPROC; i++)
    {
        _blockQ[i] = EMPTY;
    }

    /* check process table for blocked processes */
    for (int i = 0; i < MAXPROC; i++)
    {
        ScisSosPCB *pcb = _proctable[i];

        if (pcb != NULL && pcb->ps_state == PS_BLK)
        {
            if (pcb->pid >= 1 && pcb->pid <= MAXPROC)
            {
                _blockQ[b_index++] = pcb->pid;
            }
        }
    }
}

/* count ready processes */
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

/* check for active processes
   NOTE: This function is non-static so it matches the header and is linkable */
int scissos_active_processes(void)
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
                return 1; /* Active process found */
            }
        }
    }
    return 0; /* No active processes */
}

/* Move blocked process to ready state.
   Also set the ready-enter timestamp for accurate wait time accounting. */
void scissos_unblock_process(void)
{
    for (int i = 0; i < MAXPROC; i++)
    {
        if (_blockQ[i] != EMPTY)
        {
            int pid = _blockQ[i];

            /* Add bounds check */
            if (pid < 1 || pid > MAXPROC)
            {
                fprintf(stderr, "Warning: Invalid PID %d in block queue\n", pid);
                continue;
            }

            ScisSosPCB *pcb = _proctable[pid - 1];

            if (pcb != NULL && pcb->ps_state == PS_BLK)
            {
                pcb->ps_state = PS_RDY;

                /* record ready-enter time */
                _ready_enter_time[pid - 1] = now_us();

                fprintf(stdout, "[UNBLOCKED] Process PID %d moved to READY state\n", pcb->pid);
            }
        }
    }
}

/* Call the scheduler to manage processes */
void scissos_call_scheduler(char *scheduler)
{
    fprintf(stdout, "\n=== SCHEDULER INVOKED ===\n");

    /* unblock processes */
    scissos_unblock_process();

    /* update ready queue and block queue */
    scissos_update_queues();

    int ready_count = scissos_count_ready_processes();

    fprintf(stdout, "Number of ready processes: %d\n", ready_count);

    /* Print Ready queue */
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

    /* check for active processes */
    if (!scissos_active_processes())
    {
        fprintf(stdout, "No active processes --- Scheduler terminating\n");
        fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
        return;
    }

    /* Change current running process to READY (if exists).
       Also record the time it re-entered READY for wait-time accounting. */
    if (_currentPID != EMPTY && _currentPID > 0 && _currentPID <= MAXPROC)
    {
        ScisSosPCB *current_pcb = _proctable[_currentPID - 1];
        if (current_pcb != NULL && current_pcb->ps_state == PS_RUN)
        {
            current_pcb->ps_state = PS_RDY;
            _ready_enter_time[_currentPID - 1] = now_us();
        }
    }

    /* call scheduling_algo */
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

    /* check for valid selected_pid */
    if (selected_pid <= 0 || selected_pid > MAXPROC || _proctable[selected_pid - 1] == NULL)
    {
        fprintf(stdout, "Invalid process with PID %d selected for scheduling\n", selected_pid);
        fprintf(stdout, "Scheduler terminating\n");
        fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");
        return;
    }

    fprintf(stdout, "\n[SCHEDULED] Process %d selected for execution\n", selected_pid);

    /* Accumulate wait time: if we have a ready-enter timestamp for this PID,
       compute how long it waited and add to pcb->timing.wait_time_us. */
    ScisSosPCB *sel_pcb = _proctable[selected_pid - 1];
    if (sel_pcb != NULL)
    {
        long enter = _ready_enter_time[selected_pid - 1];
        if (enter != 0)
        {
            long diff = now_us() - enter;
            if (diff > 0)
            {
                sel_pcb->timing.wait_time_us += diff;
            }
            /* clear timestamp so we don't double-count */
            _ready_enter_time[selected_pid - 1] = 0;
        }

        /* update process state to running */
        sel_pcb->ps_state = PS_RUN;
    }

    _currentPID = selected_pid;

    fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");

    /* Run the selected process */
    scissos_proc_run(selected_pid, scheduler);
}
