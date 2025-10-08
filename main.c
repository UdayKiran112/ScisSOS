#include "ScisSos.h"
#include "scheduling_algo.h"

#define NUM_PROCESSES 10

// Function to create 10 processes with different characteristics
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

    printf("\n=== Process Creation Complete ===\n");
    printf("Total processes created: %d\n\n", NUM_PROCESSES);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <scheduler_name>\n", argv[0]);
        return 1;
    }

    // Step 1: Initialize OS
    scissos_initialise();

    // Step 2: Create processes
    printf("=== Creating Processes ===\n\n");
    ScisSosProcess *processes[NUM_PROCESSES];
    create_processes(processes);

    // Step 3: Print initial PCBs
    printf("=== Initial Process Control Blocks ===\n");
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        if (processes[i])
            scissos_print_pcb(processes[i], stdout);
    }

    // Step 4: Start scheduling
    printf("\n*****************************************************\n");
    printf("*          Starting Process Scheduling             *\n");
    printf("*****************************************************\n");
    scissos_call_scheduler(argv[1]);

    // Step 5: Final statistics
    printf("\n*****************************************************\n");
    printf("*              Simulation Complete                  *\n");
    printf("*****************************************************\n\n");

    printf("=== Final Process States ===\n");
    const char *state_str[] = {"NEW", "READY", "RUNNING", "BLOCKED",
                               "SUSP_READY", "SUSP_BLOCKED", "DEAD"};

    int dead_count = 0, active_count = 0;
    for (int i = 1; i < MAXPROC; i++)
    {
        if (_proctable[i])
        {
            printf("Process %d: %s (PC=%d/%d)\n",
                   i, state_str[_proctable[i]->ps_state],
                   _proctable[i]->pc, _proctable[i]->size);

            if (_proctable[i]->ps_state == PS_DEAD)
                dead_count++;
            else
                active_count++;
        }
    }

    printf("\nCompleted Processes: %d\n", dead_count);
    printf("Active Processes: %d\n", active_count);

    // Step 6: Cleanup
    printf("\n=== Cleaning up resources ===\n");
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        if (processes[i])
        {
            scissos_proc_delete(processes[i]->_PID);
            free(processes[i]);
        }
    }

    printf("\nSimulation terminated successfully.\n\n");
    return 0;
}
