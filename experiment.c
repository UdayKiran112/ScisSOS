/* experiment.c
 *
 * Experiment driver for ScisSOS:
 *  - EXP=1 : Run experiment 1 (memory OFF then ON) with given frames K
 *  - EXP=2 : Run experiment 2 (varying K) (frames arg ignored)
 *
 * Notes:
 *  - This file uses the existing ScisSos API declared in ScisSos.h.
 *  - Cleanup safely deletes any remaining processes by scanning _proctable[]
 *    instead of calling delete on process pointers that the OS may already
 *    have freed during simulation.
 */

#include "ScisSos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PROCESSES 10

/* store created process pointers so we can print later (may be freed by OS) */
static ScisSosProcess *created[NUM_PROCESSES] = {0};

/* Helper: create the 10 processes (different types) and assign memory behavior */
static void create_10_processes(void)
{
    int idx = 0;
    created[idx++] = scissos_proc_create("WebBrowser", 50, 5, PT_IOE);
    if (created[0] && created[0]->_pcb)
        created[0]->_pcb->m_type = MT_GOOD;

    created[idx++] = scissos_proc_create("Calculator", 20, 10, PT_CMP);
    if (created[1] && created[1]->_pcb)
        created[1]->_pcb->m_type = MT_GOOD;

    created[idx++] = scissos_proc_create("TextEditor", 40, 8, PT_REG);
    if (created[2] && created[2]->_pcb)
        created[2]->_pcb->m_type = MT_GOOD;

    created[idx++] = scissos_proc_create("VideoPlayer", 60, 3, PT_IOE);
    if (created[3] && created[3]->_pcb)
        created[3]->_pcb->m_type = MT_BAD;

    created[idx++] = scissos_proc_create("Compiler", 80, 15, PT_CMP);
    if (created[4] && created[4]->_pcb)
        created[4]->_pcb->m_type = MT_BAD;

    created[idx++] = scissos_proc_create("FileManager", 35, 7, PT_REG);
    if (created[5] && created[5]->_pcb)
        created[5]->_pcb->m_type = MT_UGLY;

    created[idx++] = scissos_proc_create("Database", 70, 4, PT_IOE);
    if (created[6] && created[6]->_pcb)
        created[6]->_pcb->m_type = MT_UGLY;

    created[idx++] = scissos_proc_create("Game", 100, 2, PT_CMP);
    if (created[7] && created[7]->_pcb)
        created[7]->_pcb->m_type = MT_UGLY;

    created[idx++] = scissos_proc_create("Terminal", 25, 12, PT_REG);
    if (created[8] && created[8]->_pcb)
        created[8]->_pcb->m_type = MT_GOOD;

    created[idx++] = scissos_proc_create("ImageEditor", 55, 6, PT_IOE);
    if (created[9] && created[9]->_pcb)
        created[9]->_pcb->m_type = MT_BAD;

    fprintf(stdout, "=== Process Creation Complete ===\n");
    fprintf(stdout, "Total processes created: %d\n", NUM_PROCESSES);
}

/* Print initial PCBs for debugging/observation using created[] */
static void print_initial_pcbs(void)
{
    fprintf(stdout, "\n=== Initial Process Control Blocks ===\n");
    for (int i = 0; i < NUM_PROCESSES; ++i)
    {
        if (created[i] != NULL)
        {
            /* scissos_print_pcb should handle internal nulls, but guard anyway */
            scissos_print_pcb(created[i], stdout);
            fprintf(stdout, "\n");
        }
    }
}

/* Print timing info for all completed processes (uses global _proctable of PCBs) */
static void print_timing_and_memory_stats(void)
{
    fprintf(stdout, "\n=== Memory Manager Statistics ===\n");
    /* Only print memory stats when memory manager is enabled and physical memory exists */
    if (_memory_manager_enabled && _physical_memory != NULL)
    {
        memory_print_stats(stdout);
    }
    else
    {
        fprintf(stdout, "Memory manager disabled or not initialised — skipping memory stats.\n");
    }

    fprintf(stdout, "\n=== Process Timing Information ===\n");
    for (int i = 0; i < MAXPROC; ++i)
    {
        if (_proctable[i] != NULL)
        {
            if (_proctable[i]->ps_state == PS_DEAD)
                scissos_print_timing_info(_proctable[i], stdout);
        }
    }
}

/* Cleanup: delete any remaining processes by scanning _proctable[] (safe) */
/* Also clear created[] pointers without dereferencing freed memory */
static void cleanup_all(void)
{
    /* Delete any processes that remain in the global proctable.
       This is safe: scissos_proc_delete expects a pid and proctable entry
       is the authoritative source of alive processes. */
    for (int i = 0; i < MAXPROC; ++i)
    {
        if (_proctable[i] != NULL)
        {
            int pid = _proctable[i]->pid;
            /* scissos_proc_delete should be safe if pid valid */
            scissos_proc_delete(pid);
        }
    }

    /* Nullify our created[] references to avoid accidental reuse */
    for (int i = 0; i < NUM_PROCESSES; ++i)
        created[i] = NULL;

    /* Cleanup memory manager (if enabled) */
    if (_memory_manager_enabled)
    {
        memory_manager_cleanup();
        _memory_manager_enabled = 0;
    }
}

/* Run a single simulation with the chosen scheduler until completion */
static void run_simulation(const char *sched)
{
    int iteration = 0;
    const int MAX_ITER = 20000; /* safety cap */

    /* Re-initialize OS for each simulation run so proctable/queues are fresh */
    scissos_initialise();

    create_10_processes();
    print_initial_pcbs();

    while (scissos_active_processes())
    {
        ++iteration;
        fprintf(stdout, "\n--- Scheduling Iteration %d ---\n", iteration);
        scissos_call_scheduler((char *)sched); /* API expects char * */
        if (iteration > MAX_ITER)
        {
            fprintf(stderr, "[ERROR] Reached maximum iterations (%d). Aborting run.\n", MAX_ITER);
            break;
        }
    }

    fprintf(stdout, "\n[INFO] All processes completed after %d scheduling iterations\n", iteration);

    print_timing_and_memory_stats();

    /* After run, clear any leftover processes and memory structures */
    cleanup_all();
}

/* EXPERIMENT 1:
 *  - Run A: memory manager OFF
 *  - Run B: memory manager ON
 */
static void experiment1(const char *sched, int K)
{
    fprintf(stdout, "\n============================================\n");
    fprintf(stdout, " Experiment 1 (K = %d) : MEMORY OFF then MEMORY ON\n", K);
    fprintf(stdout, "============================================\n\n");

    /* Run A: Memory manager OFF (do not initialize memory manager) */
    _memory_manager_enabled = 0;
    _num_frames = K;
    fprintf(stdout, "\n--- Run A : MEMORY MANAGER OFF (K=%d) ---\n", K);
    run_simulation(sched);

    /* Ensure memory manager state is clean */
    if (_memory_manager_enabled)
    {
        memory_manager_cleanup();
        _memory_manager_enabled = 0;
    }

    /* Run B: Memory manager ON */
    _memory_manager_enabled = 1;
    _num_frames = K;

    /* Initialize memory structures explicitly before the run (safe) */
    memory_manager_init(_num_frames);

    fprintf(stdout, "\n--- Run B : MEMORY MANAGER ON (K=%d) ---\n", K);
    run_simulation(sched);

    /* Cleanup after experiment */
    if (_memory_manager_enabled)
    {
        memory_manager_cleanup();
        _memory_manager_enabled = 0;
    }
}

/* EXPERIMENT 2:
 *  - Vary K from 32 to 128 in steps of 16, memory manager ON for each K.
 */
static void experiment2_vary_K(const char *sched)
{
    fprintf(stdout, "\n============================================\n");
    fprintf(stdout, " Experiment 2 : Varying K = 32..128 step 16 (memory manager ON)\n");
    fprintf(stdout, "============================================\n");

    for (int K = 32; K <= 128; K += 16)
    {
        fprintf(stdout, "\n\n########## Running with K = %d ##########\n", K);
        _memory_manager_enabled = 1;
        _num_frames = K;
        memory_manager_init(_num_frames);

        run_simulation(sched);

        if (_memory_manager_enabled)
        {
            memory_manager_cleanup();
            _memory_manager_enabled = 0;
        }
    }
}

/* Main: parse arguments and dispatch */
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stdout, "Usage: %s <scheduler> <EXP> <FRAMES>\n", argv[0]);
        fprintf(stdout, "  <scheduler> : fcfs | sjf | priority | rr\n");
        fprintf(stdout, "  <EXP>       : 1 (experiment1) | 2 (experiment2 vary K)\n");
        fprintf(stdout, "  <FRAMES>    : frames (K) for EXP=1; ignored for EXP=2 (varying K)\n");
        fprintf(stdout, "Examples:\n");
        fprintf(stdout, "  %s rr 1 64      -> Experiment1 with K=64 (OFF then ON)\n", argv[0]);
        fprintf(stdout, "  %s rr 2 64      -> Experiment2 (varies K from 32..128 step 16)\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sched = argv[1];
    int EXP = atoi(argv[2]);
    int K = atoi(argv[3]);
    if (K <= 0)
        K = NUMFRAMES; /* fallback */

    if (strcmp(sched, "fcfs") != 0 && strcmp(sched, "sjf") != 0 &&
        strcmp(sched, "priority") != 0 && strcmp(sched, "rr") != 0)
    {
        fprintf(stderr, "Error: Unknown scheduler '%s'. Use: fcfs, sjf, priority, rr\n", sched);
        return EXIT_FAILURE;
    }

    if (EXP == 1)
    {
        experiment1(sched, K);
    }
    else if (EXP == 2)
    {
        experiment2_vary_K(sched);
    }
    else
    {
        fprintf(stderr, "Error: EXP must be 1 or 2\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
