/* main.c - corrected to match ScisSos API names and headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ScisSos.h"
#include "scheduling_algo.h"

#define NUM_PROCESSES 10

/* Function to create 10 processes with different characteristics */
void create_processes(ScisSosProcess *processes[])
{
    processes[0] = scissos_proc_create("WebBrowser", 50, 5, PT_IOE);
    processes[1] = scissos_proc_create("Calculator", 20, 10, PT_CMP);
    processes[2] = scissos_proc_create("TextEditor", 40, 8, PT_REG);
    processes[3] = scissos_proc_create("VideoPlayer", 60, 3, PT_IOE);
    processes[4] = scissos_proc_create("Compiler", 80, 15, PT_CMP);
    processes[5] = scissos_proc_create("FileManager", 35, 7, PT_REG);
    processes[6] = scissos_proc_create("Database", 70, 4, PT_IOE);
    processes[7] = scissos_proc_create("Game", 100, 2, PT_CMP);
    processes[8] = scissos_proc_create("Terminal", 25, 12, PT_REG);
    processes[9] = scissos_proc_create("ImageEditor", 55, 6, PT_IOE);

    fprintf(stdout, "\n=== Process Creation Complete ===\n");
    fprintf(stdout, "Total processes created: %d\n\n", NUM_PROCESSES);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <scheduler_name>\n", argv[0]);
        fprintf(stdout, "Available schedulers: fcfs, sjf, priority, rr\n");
        return EXIT_FAILURE;
    }

    /* Step 1: Initialize OS (this also initialises memory manager if enabled) */
    scissos_initialise();

    /* Step 2: Create processes */
    fprintf(stdout, "=== Creating Processes ===\n\n");
    ScisSosProcess *processes[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; ++i)
        processes[i] = NULL;
    create_processes(processes);

    /* Step 3: Print initial PCBs */
    fprintf(stdout, "=== Initial Process Control Blocks ===\n");
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        if (processes[i])
        {
            scissos_print_pcb(processes[i], stdout);
            fprintf(stdout, "\n");
        }
    }

    /* Validate scheduler choice */
    int valid_scheduler = 0;
    if (strcmp(argv[1], "fcfs") == 0 || strcmp(argv[1], "sjf") == 0 ||
        strcmp(argv[1], "priority") == 0 || strcmp(argv[1], "rr") == 0)
    {
        valid_scheduler = 1;
    }

    if (!valid_scheduler)
    {
        fprintf(stderr, "Error: Unknown scheduler '%s'\n", argv[1]);
        fprintf(stdout, "Available schedulers: fcfs, sjf, priority, rr\n");
        return EXIT_FAILURE;
    }

    /* Step 4: Start scheduling loop */
    fprintf(stdout, "=== Starting Scheduling with '%s' Algorithm ===\n", argv[1]);

    /* Loop until all processes are completed */
    int iteration = 0;

    /* NOTE: Use the API function from ScisSos.h that reports active processes.
       Ensure the function name matches your header (scissos_active_processes). */
    while (scissos_active_processes())
    {
        iteration++;
        fprintf(stdout, "\n--- Scheduling Iteration %d ---\n", iteration);

        scissos_call_scheduler(argv[1]);

        /* Safety check to prevent infinite loops */
        if (iteration > 10000)
        {
            fprintf(stderr, "\n[ERROR] Maximum iterations exceeded!\n");
            fprintf(stderr, "Possible infinite loop detected.\n");
            break;
        }
    }

    printf("\n[INFO] All processes completed after %d scheduling iterations\n", iteration);

    /* Step 5: Final statistics */
    fprintf(stdout, "\n=== Final Statistics ===\n");

    fprintf(stdout, "=== Final Process States ===\n");
    const char *state_str[] = {"NEW", "READY", "RUNNING", "BLOCKED",
                               "SUSP_READY", "SUSP_BLOCKED", "DEAD"};

    int dead_count = 0, active_count = 0;
    for (int i = 0; i < MAXPROC; i++)
    {
        if (_proctable[i] != NULL)
        {
            fprintf(stdout, "Process %d: %s (PC=%d/%d)\n",
                    _proctable[i]->pid,
                    (_proctable[i]->ps_state >= 0 && _proctable[i]->ps_state <= PS_DEAD) ? state_str[_proctable[i]->ps_state] : "UNKNOWN",
                    _proctable[i]->pc,
                    _proctable[i]->size);

            if (_proctable[i]->ps_state == PS_DEAD)
                dead_count++;
            else
                active_count++;
        }
    }

    fprintf(stdout, "\nCompleted Processes: %d\n", dead_count);
    fprintf(stdout, "Active Processes: %d\n", active_count);

    /* Print memory statistics if memory manager is enabled */
    if (_memory_manager_enabled)
    {
        memory_print_stats(stdout);

        /* Print timing info for all processes */
        fprintf(stdout, "\n=== Process Timing Information ===\n");
        for (int i = 0; i < MAXPROC; i++)
        {
            if (_proctable[i] != NULL && _proctable[i]->ps_state == PS_DEAD)
            {
                scissos_print_timing_info(_proctable[i], stdout);
            }
        }
    }

    /* Step 6: Cleanup */
    fprintf(stdout, "\n=== Cleaning up resources ===\n");

    /* Delete all processes */
    for (int i = 0; i < MAXPROC; i++)
    {
        if (_proctable[i] != NULL)
        {
            int pid = _proctable[i]->pid;
            scissos_proc_delete(pid);
        }
    }

    /* Cleanup memory manager */
    if (_memory_manager_enabled)
    {
        memory_manager_cleanup();
    }

    fprintf(stdout, "\nSimulation terminated successfully.\n\n");
    return EXIT_SUCCESS;
}
