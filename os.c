#include "scheduling_algo.h"
#include <time.h>

// Global variables
int _currentPID = EMPTY;
ScisSosPCB *_proctable[MAXPROC] = {NULL};
int _readyQ[MAXPROC] = {EMPTY};
int _blockQ[MAXPROC] = {EMPTY};
struct timeval base_time;
Time process_times[MAXPROC];
Frame frame_table[NUMFRAMES];

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

    gettimeofday(&base_time, NULL);

    fprintf(stdout, "Base time set is : %ld seconds and %ld microseconds\n", (long)base_time.tv_sec, (long)base_time.tv_usec);

    // Initialize memory management here
    memory_initialise();

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
                _blockQ[i] = EMPTY;
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

    struct timeval curr_time; // current time

    // Change current running process to READY (if exists)
    if (_currentPID != EMPTY && _currentPID > 0 && _currentPID <= MAXPROC)
    {
        ScisSosPCB *current_pcb = _proctable[_currentPID - 1];
        if (current_pcb != NULL && current_pcb->ps_state == PS_RUN)
        {
            current_pcb->ps_state = PS_RDY;
            gettimeofday(&curr_time, NULL);
            process_times[_currentPID - 1].last_ready_time = curr_time;
            fprintf(stdout, "[CONTEXT SWITCH] Process PID %d moved to READY state\n", _currentPID);
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

    // Response time update
    gettimeofday(&curr_time, NULL);
    if (process_times[selected_pid - 1].response_flag == 0)
    {
        process_times[selected_pid - 1].first_scheduled_time = curr_time;
        process_times[selected_pid - 1].response_flag = 1;

        struct timeval response = difference_times(process_times[selected_pid - 1].create_time, curr_time);
        fprintf(stdout, "[RESPONSE TIME] Process PID %d response time: %f ms\n",
                selected_pid,
                timeval_to_seconds(response) * 1000);
    }

    // Waiting time update
    if (process_times[selected_pid - 1].last_ready_time.tv_sec != 0 || process_times[selected_pid - 1].last_ready_time.tv_usec != 0)
    {
        struct timeval wait_diff = difference_times(process_times[selected_pid - 1].last_ready_time, curr_time);
        process_times[selected_pid - 1].wait_time = add_times(process_times[selected_pid - 1].wait_time, wait_diff);
        fprintf(stdout, "[WAITING TIME] Process PID %d total waiting time updated to: %f mss\n",
                selected_pid,
                timeval_to_seconds(process_times[selected_pid - 1].wait_time));
    }

    fprintf(stdout, "=== SCHEDULER TERMINATED ===\n");

    // Run the selected process
    scissos_proc_run(selected_pid, scheduler);
}

struct timeval difference_times(struct timeval start, struct timeval end)
{
    struct timeval diff;

    if ((end.tv_usec - start.tv_usec) < 0)
    {
        diff.tv_sec = end.tv_sec - start.tv_sec - 1;
        diff.tv_usec = end.tv_usec - start.tv_usec + 1000000;
    }
    else
    {
        diff.tv_sec = end.tv_sec - start.tv_sec;
        diff.tv_usec = end.tv_usec - start.tv_usec;
    }

    return diff;
}

struct timeval add_times(struct timeval t1, struct timeval t2)
{
    struct timeval sum;
    sum.tv_sec = t1.tv_sec + t2.tv_sec;
    sum.tv_usec = t1.tv_usec + t2.tv_usec;

    if (sum.tv_usec >= 1000000)
    {
        sum.tv_sec += sum.tv_usec / 1000000;
        sum.tv_usec = sum.tv_usec % 1000000;
    }
    return sum;
}

double timeval_to_seconds(struct timeval t)
{
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}

void print_time(struct timeval t)
{
    fprintf(stdout, "%ld seconds and %ld microseconds\n", t.tv_sec, t.tv_usec);
}