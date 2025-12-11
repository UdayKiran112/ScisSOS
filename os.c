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

// Memory management global variables
int _memory_enabled = 0;     // Initialize here
int page_faults = 0;         // Initialize here
int _total_instructions = 0; // Declare and initialize _total_instructions

/* Map process state number → human-readable text */
const char *get_state_name(int state)
{
    switch (state)
    {
    case PS_NEW:
        return "NEW";
    case PS_RDY:
        return "READY";
    case PS_RUN:
        return "RUNNING";
    case PS_BLK:
        return "BLOCKED";
    case PS_SRDY:
        return "SUSP_READY";
    case PS_SBLK:
        return "SUSP_BLOCKED";
    case PS_DEAD:
        return "DEAD";
    default:
        return "UNKNOWN";
    }
}

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

    // end - start (not start - end)
    diff.tv_sec = end.tv_sec - start.tv_sec;
    diff.tv_usec = end.tv_usec - start.tv_usec;

    // Handle negative microseconds
    if (diff.tv_usec < 0)
    {
        diff.tv_sec--;
        diff.tv_usec += 1000000;
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

/* Place this in os.c (or times.c) and compile with the rest of the project. */
#include <stdio.h>

/* small helper: true if timeval is non-zero */
static inline int timeval_is_set(struct timeval t)
{
    return (t.tv_sec != 0 || t.tv_usec != 0);
}

/* helper to convert timeval to milliseconds */
double tv_to_ms(struct timeval t)
{
    return timeval_to_seconds(t) * 1000.0;
};

/* Print timing statistics (uses process_times[], difference_times(), timeval_to_seconds()) */
void scissos_print_timings(void)
{
    int i;
    struct timeval response_tv, turnaround_tv;

    printf("\n========================================\n");
    printf("   PROCESS TIMING STATISTICS\n");
    printf("========================================\n");
    printf("%-5s %-15s %-15s %-15s %-15s\n",
           "PID", "Response(ms)", "Turnaround(ms)", "Wait(ms)", "Status");
    printf("------------------------------------------------------------------------\n");

    for (i = 0; i < MAXPROC; i++)
    {
        if (_proctable[i] != NULL)
        {
            /* Defensive: make sure index i maps to process_times[] correctly.
               In your code process_times is indexed by pid-1 in many places,
               but earlier prints used table index i; to remain consistent with
               your provided snippet we use index i here. If your pid is stored
               as 1..N and you want to index by pid-1, change process_times[i]
               to process_times[_proctable[i]->pid - 1]. */
            Time t = process_times[i];

            /* Response time = first_scheduled_time - create_time (if scheduled) */
            if (t.response_flag && (t.first_scheduled_time.tv_sec || t.first_scheduled_time.tv_usec))
            {
                response_tv = difference_times(t.create_time, t.first_scheduled_time);
            }
            else
            {
                response_tv.tv_sec = 0;
                response_tv.tv_usec = 0;
            }

            /* Turnaround = terminate_time - create_time (only if terminated) */
            if (_proctable[i]->ps_state == PS_DEAD &&
                (t.terminate_time.tv_sec || t.terminate_time.tv_usec))
            {
                turnaround_tv = difference_times(t.create_time, t.terminate_time);

                printf("P%-4d %-15.2f %-15.2f %-15.2f COMPLETED\n",
                       _proctable[i]->pid,
                       tv_to_ms(response_tv),
                       tv_to_ms(turnaround_tv),
                       tv_to_ms(t.wait_time));
            }
            else
            {
                printf("P%-4d %-15.2f %-15s %-15.2f %s\n",
                       _proctable[i]->pid,
                       tv_to_ms(response_tv),
                       "N/A",
                       tv_to_ms(t.wait_time),
                       get_state_name(_proctable[i]->ps_state));
            }
        }
    }

    printf("========================================\n");
    printf("Total Page Faults: %d\n", page_faults);
    printf("========================================\n\n");
}
