#include "ScisSos.h"
#include <time.h>

#define NUM_PROCESSES 10

// Function to create processes with different characteristics
void create_processes(ScisSosProcess *processes[])
{
    processes[0] = scissos_proc_create("WebBrowser", 30, 5, PT_REG, MT_GOOD);
    processes[1] = scissos_proc_create("Calculator", 30, 10, PT_REG, MT_BAD);
    processes[2] = scissos_proc_create("TextEditor", 30, 8, PT_REG, MT_GOOD);
    processes[3] = scissos_proc_create("VideoPlayer", 30, 3, PT_REG, MT_UGLY);
    processes[4] = scissos_proc_create("Compiler", 30, 15, PT_REG, MT_BAD);
    processes[5] = scissos_proc_create("FileManager", 30, 7, PT_REG, MT_GOOD);
    processes[6] = scissos_proc_create("Database", 30, 4, PT_REG, MT_UGLY);
    processes[7] = scissos_proc_create("Game", 30, 2, PT_REG, MT_BAD);
    processes[8] = scissos_proc_create("Terminal", 30, 12, PT_REG, MT_UGLY);
    processes[9] = scissos_proc_create("ImageEditor", 30, 6, PT_REG, MT_UGLY);

    fprintf(stdout, "\n========================================\n");
    fprintf(stdout, "   ALL PROCESSES CREATED\n");
    fprintf(stdout, "   Total Processes: %d\n", NUM_PROCESSES);
    fprintf(stdout, "========================================\n\n");
}

int main(int argc, char *argv[])
{
    ScisSosProcess *processes[NUM_PROCESSES];
    int i;
    int memory_mode = 1; /* 1 = enabled, 0 = disabled */
    char *scheduler_name = "fcfs";

    /* Seed random number generator */
    srand(time(NULL));

    /* Parse command line arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "no-memory") == 0)
        {
            memory_mode = 0;
        }
        else if (strcmp(argv[i], "sched") == 0)
        {
            if (i + 1 < argc)
            {
                i++;
                scheduler_name = argv[i];

                // Validate scheduler name
                if (strcmp(scheduler_name, "fcfs") != 0 &&
                    strcmp(scheduler_name, "sjf") != 0 &&
                    strcmp(scheduler_name, "priority") != 0 &&
                    strcmp(scheduler_name, "rr") != 0)
                {
                    fprintf(stderr, "Unknown scheduling algorithm: %s\n", scheduler_name);
                    fprintf(stdout, "Available schedulers: fcfs, sjf, priority, rr\n");
                    return 1;
                }
            }
        }
    }

    printf("\n============ OS Simulator ================\n\n");

    /* Initialize OS */
    scissos_initialise();
    _memory_enabled = memory_mode;

    printf("Configuration:\n");
    if (_memory_enabled)
    {
        printf("  Memory Management: ENABLED\n");
        printf("  Page Replacement: FIFO\n");
    }
    else
    {
        printf("  Memory Management: DISABLED\n");
    }

    printf("  Scheduling Algorithm: %s\n", scheduler_name);
    printf("  Number of Frames: %d\n", NUMFRAMES);
    printf("  Page Size: %d bytes\n", PAGESIZE);
    printf("  Time Slice: %d instructions\n", DEFTS);
    printf("\n");

    /* Create processes */
    printf("===================== CREATING PROCESSES ===============================\n\n");

    create_processes(processes);

    /* Print initial PCBs */
    printf("===================== INITIAL PROCESS STATES ===========================\n");
    for (i = 0; i < NUM_PROCESSES; i++)
    {
        if (processes[i])
        {
            scissos_print_pcb(processes[i], stdout);
            fprintf(stdout, "\n");
        }
    }

    /* Start scheduling */
    printf("===================== STARTING SCHEDULER ===============================\n");
    printf("Starting scheduler with '%s' algorithm...\n\n", scheduler_name);

    /* Loop until all processes are completed */
    int iteration = 0;
    while (scisos_active_processes())
    {
        iteration++;
        fprintf(stdout, "\n--- Scheduling Iteration %d ---\n", iteration);

        scissos_call_scheduler(scheduler_name);

        /* Safety check to prevent infinite loops */
        if (iteration > 10000)
        {
            fprintf(stderr, "\n[ERROR] Maximum iterations exceeded!\n");
            fprintf(stderr, "Possible infinite loop detected.\n");
            break;
        }
    }

    printf("\n\n[INFO] All processes completed after %d scheduling iterations\n", iteration);

    /* write timings*/
    scissos_print_timings();

    /* Print final statistics */
    printf("\n===================== FINAL STATISTICS =================================\n");

    printf("\n=== Final Process States ===\n");
    const char *state_str[] = {"NEW", "READY", "RUNNING", "BLOCKED",
                               "SUSP_READY", "SUSP_BLOCKED", "DEAD"};

    int dead_count = 0, active_count = 0;
    for (i = 0; i < MAXPROC; i++)
    {
        if (_proctable[i] != NULL)
        {
            fprintf(stdout, "Process PID %d: %s (PC=%d/%d)\n",
                    _proctable[i]->pid, state_str[_proctable[i]->ps_state],
                    _proctable[i]->pc, _proctable[i]->size);

            if (_proctable[i]->ps_state == PS_DEAD)
                dead_count++;
            else
                active_count++;
        }
    }

    fprintf(stdout, "\nCompleted Processes: %d\n", dead_count);
    fprintf(stdout, "Active Processes: %d\n", active_count);

    /* Print memory statistics if enabled */
    if (_memory_enabled)
    {
        print_memory_stats();
    }

    /* Cleanup */
    printf("\n======================= CLEANUP ========================================\n");
    for (i = 0; i < MAXPROC; i++)
    {
        if (_proctable[i] != NULL)
        {
            int pid = _proctable[i]->pid;
            fprintf(stdout, "Deleting process PID %d...\n", pid);
            scissos_proc_delete(pid);
        }
    }

    printf("Cleanup complete!\n");
    printf("========================================================================\n\n");
    printf("Simulation terminated successfully.\n\n");

    return 0;
}