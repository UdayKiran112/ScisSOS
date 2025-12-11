/* process.c - corrected and complete version */

#include "ScisSos.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static int pid_counter = 1;

/* Number of bytes we simulate per "instruction".
   Tune this (or PAGESIZE) to get processes spanning multiple pages
   in your experiments. */
#ifndef INSTR_BYTES
#define INSTR_BYTES 512
#endif

/* Calculate time difference in microseconds */
long get_time_diff_us(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
}

/* Generate code for a process based on its type */
ScisSosInst **scissos_generate_code(int size, int p_type)
{
    if (size <= 0)
    {
        fprintf(stderr, "Error: Invalid size %d for code generation.\n", size);
        return NULL;
    }

    ScisSosInst **code = (ScisSosInst **)malloc(size * sizeof(ScisSosInst *));
    if (!code)
    {
        fprintf(stderr, "Error: Memory allocation failed for process code.\n");
        return NULL;
    }

    double long_call_prob;
    switch (p_type)
    {
    case PT_CMP:
        long_call_prob = CMP_THR;
        break;
    case PT_IOE:
        long_call_prob = IOE_THR;
        break;
    default:
        long_call_prob = REG_THR;
        break;
    }

    for (int i = 0; i < size; i++)
    {
        code[i] = (ScisSosInst *)malloc(sizeof(ScisSosInst));
        if (!code[i])
        {
            fprintf(stderr, "Error: Memory allocation failed for instruction %d.\n", i);
            for (int j = 0; j < i; j++)
            {
                free(code[j]);
            }
            free(code);
            return NULL;
        }

        code[i]->_inum = i;

        /* Determine if this is a long or short system call */
        double rand_val = (double)rand() / (double)RAND_MAX;
        if (rand_val < long_call_prob)
        {
            code[i]->_syscall = INS_LNG;
        }
        else
        {
            code[i]->_syscall = INS_SHR;
        }

        /* Memory address will be set from the generated sequence */
        code[i]->_addref = 0;
    }

    return code;
}

/* Create and initialize a PCB */
void scissos_create_pcb(ScisSosProcess *process, int pid, int uid, int size,
                        int priority, int p_type, int m_type, ScisSosInst **code)
{
    if (process == NULL)
    {
        fprintf(stderr, "Error: scissos_create_pcb called with NULL process\n");
        return;
    }

    process->_pcb = (ScisSosPCB *)malloc(sizeof(ScisSosPCB));
    if (!process->_pcb)
    {
        fprintf(stderr, "Error: Memory allocation failed for PCB.\n");
        return;
    }

    process->_pcb->pid = pid;
    process->_pcb->uid = uid;
    process->_pcb->size = size;
    process->_pcb->priority_value = priority;
    process->_pcb->ps_state = PS_NEW;
    process->_pcb->p_type = p_type;
    process->_pcb->m_type = m_type;
    process->_pcb->pc = 0;
    process->_pcb->p_code = code;
    process->_pcb->p_timeslice = DEFTS;
    process->_pcb->page_faults = 0;
    process->_pcb->reference_counter = 0;
    process->_pcb->address_sequence = NULL;

    /* Convert instruction count to total bytes and compute pages needed */
    long total_bytes = (long)size * (long)INSTR_BYTES;
    if (total_bytes <= 0)
        total_bytes = INSTR_BYTES; /* safety */

    process->_pcb->num_pages = (int)((total_bytes + PAGESIZE - 1) / PAGESIZE);
    if (process->_pcb->num_pages < 1)
        process->_pcb->num_pages = 1;
    if (process->_pcb->num_pages > MAXPGES)
        process->_pcb->num_pages = MAXPGES;

    /* Generate memory address reference sequence */
    process->_pcb->address_sequence = (int *)malloc((size_t)size * sizeof(int));
    if (process->_pcb->address_sequence == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate address sequence for PID %d\n", pid);
    }
    else
    {
        /* Generate addresses based on memory type */
        int working_set_size, transition_freq;

        switch (m_type)
        {
        case MT_GOOD:             /* Small working set, few page faults */
            working_set_size = 2; /* 2 pages */
            transition_freq = 20;
            break;
        case MT_BAD: /* Larger working set, more page faults */
            working_set_size = 4;
            transition_freq = 15;
            break;
        case MT_UGLY: /* Large working set, many page faults */
            working_set_size = 8;
            transition_freq = 10;
            break;
        default:
            working_set_size = 2;
            transition_freq = 20;
            break;
        }

        /* Clamp working_set_size to available pages (at least 1) */
        if (working_set_size > process->_pcb->num_pages)
            working_set_size = process->_pcb->num_pages;
        if (working_set_size < 1)
            working_set_size = 1;

        int current_working_set_base = 0;

        for (int i = 0; i < size; i++)
        {
            if (i < 100)
            {
                /* Spread the initial locality across the process's pages
                   (not only page 0) to avoid only-first-page references. */
                int max_addr_space = process->_pcb->num_pages * PAGESIZE;
                if (max_addr_space <= 0)
                    max_addr_space = PAGESIZE;
                process->_pcb->address_sequence[i] = rand() % max_addr_space;
            }
            else
            {
                /* Change working set base periodically (transition phase) */
                if (i % transition_freq == 0)
                {
                    int max_base;
                    if (process->_pcb->num_pages > working_set_size)
                        max_base = process->_pcb->num_pages - working_set_size;
                    else
                        max_base = 0;

                    if (max_base == 0)
                        current_working_set_base = 0;
                    else
                        current_working_set_base = rand() % (max_base + 1);
                }

                /* Generate address within current working set (stable phase) */
                int page_offset = rand() % working_set_size;
                int page = current_working_set_base + page_offset;

                /* Ensure page is within bounds */
                if (page >= process->_pcb->num_pages)
                    page = process->_pcb->num_pages - 1;
                if (page < 0)
                    page = 0;

                process->_pcb->address_sequence[i] = page * PAGESIZE + (rand() % PAGESIZE);
            }
        }

        /* Copy addresses to instruction address references */
        for (int i = 0; i < size; i++)
        {
            if (code && code[i])
                code[i]->_addref = process->_pcb->address_sequence[i];
        }

        fprintf(stdout,
                "[MEMORY] Generated address sequence for PID %d (type=%s, size=%d, pages=%d)\n",
                pid,
                m_type == MT_GOOD ? "GOOD" : (m_type == MT_BAD ? "BAD" : "UGLY"),
                size,
                process->_pcb->num_pages);
    }

    /* Initialize timing information */
    gettimeofday(&process->_pcb->timing.creation_time, NULL);
    process->_pcb->timing.first_schedule_time.tv_sec = 0;
    process->_pcb->timing.first_schedule_time.tv_usec = 0;
    process->_pcb->timing.completion_time.tv_sec = 0;
    process->_pcb->timing.completion_time.tv_usec = 0;
    process->_pcb->timing.response_time_us = 0;
    process->_pcb->timing.running_time_us = 0;
    process->_pcb->timing.wait_time_us = 0;
    process->_pcb->timing.scheduled_count = 0;

    /* Page table initialization */
    for (int i = 0; i < MAXPGES; i++)
    {
        process->_pcb->pg_table[i][0] = i;
        process->_pcb->pg_table[i][1] = EMPTY;
    }
}

/* Create a new process */
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type)
{
    if (size <= 0)
    {
        fprintf(stderr, "Error: Invalid size %d. Must be positive.\n", size);
        return NULL;
    }

    if (pid_counter > MAXPROC)
    {
        fprintf(stderr, "Error: Process table full. Cannot create more processes.\n");
        return NULL;
    }

    int pid = pid_counter++;
    int uid = (rand() % MAXUSRS) + 1;
    int m_type = MT_GOOD; /* Default memory type */

    /* Generate code for process */
    ScisSosInst **code = scissos_generate_code(size, p_type);
    if (!code)
    {
        fprintf(stderr, "Error: Failed to generate code for process.\n");
        return NULL;
    }

    /* Allocate process structure */
    ScisSosProcess *new_process = (ScisSosProcess *)malloc(sizeof(ScisSosProcess));
    if (!new_process)
    {
        fprintf(stderr, "Error: Memory allocation failed for process structure.\n");
        for (int i = 0; i < size; i++)
            free(code[i]);
        free(code);
        return NULL;
    }

    memset(new_process, 0, sizeof(ScisSosProcess));
    snprintf(new_process->_pname, sizeof(new_process->_pname), "%s", process_name);
    new_process->_PID = pid;
    new_process->_psize = size;
    new_process->_CODE = code;

    scissos_create_pcb(new_process, pid, uid, size, priority, p_type, m_type, new_process->_CODE);
    _proctable[pid - 1] = new_process->_pcb;

    /* If memory manager is enabled, optionally load first page.
       If you want demand paging (first access triggers a fault), remove this call. */
    /*if (_memory_manager_enabled)
    {
        memory_load_page(pid, 0);
    }*/

    new_process->_pcb->ps_state = PS_RDY;

    fprintf(stdout, "Process created: %s, PID: %d, UID: %d, Priority: %d, Type: %d, MemType: %d\n",
            process_name, pid, uid, priority, p_type, m_type);

    return new_process;
}

/* Print PCB information */
void scissos_print_pcb(ScisSosProcess *process, FILE *pcb_info)
{
    if (process == NULL || pcb_info == NULL || process->_pcb == NULL)
    {
        fprintf(stderr, "Error: Invalid process or pcb_info.\n");
        return;
    }

    const char *state_names[] = {"NEW", "READY", "RUNNING", "BLOCKED",
                                 "SUSP_READY", "SUSP_BLOCKED", "DEAD"};
    const char *type_names[] = {"REGULAR", "COMPUTE_INTENSIVE", "IO_INTENSIVE"};
    const char *mem_type_names[] = {"", "", "", "GOOD", "BAD", "UGLY"};

    fprintf(pcb_info, "----------------------------------------\n");
    fprintf(pcb_info, "Process Name: %s\n", process->_pname);
    fprintf(pcb_info, "PID: %d\n", process->_pcb->pid);
    fprintf(pcb_info, "UID: %d\n", process->_pcb->uid);
    fprintf(pcb_info, "Size: %d instructions\n", process->_pcb->size);
    fprintf(pcb_info, "Pages: %d\n", process->_pcb->num_pages);
    fprintf(pcb_info, "Priority: %d\n", process->_pcb->priority_value);
    fprintf(pcb_info, "State: %s (%d)\n", state_names[process->_pcb->ps_state], process->_pcb->ps_state);
    fprintf(pcb_info, "Type: %s (%d)\n", type_names[process->_pcb->p_type], process->_pcb->p_type);
    fprintf(pcb_info, "Memory Type: %s (%d)\n",
            (process->_pcb->m_type >= MT_GOOD && process->_pcb->m_type <= MT_UGLY) ? mem_type_names[process->_pcb->m_type] : "UNKNOWN",
            process->_pcb->m_type);
    fprintf(pcb_info, "Program Counter: %d\n", process->_pcb->pc);
    fprintf(pcb_info, "Time Slice: %d\n", process->_pcb->p_timeslice);
    fprintf(pcb_info, "Page Faults: %d\n", process->_pcb->page_faults);
    fprintf(pcb_info, "----------------------------------------\n");
}

/* Print timing information */
void scissos_print_timing_info(ScisSosPCB *pcb, FILE *output)
{
    if (pcb == NULL || output == NULL)
    {
        return;
    }

    const char *mem_type_names[] = {"", "", "", "GOOD", "BAD", "UGLY"};

    fprintf(output, "\n--- Timing Information for PID %d ---\n", pcb->pid);
    fprintf(output, "Memory Type: %s\n",
            (pcb->m_type >= MT_GOOD && pcb->m_type <= MT_UGLY) ? mem_type_names[pcb->m_type] : "UNKNOWN");
    fprintf(output, "Response Time: %ld microseconds (%.3f ms)\n",
            pcb->timing.response_time_us, pcb->timing.response_time_us / 1000.0);
    fprintf(output, "Running Time: %ld microseconds (%.3f ms)\n",
            pcb->timing.running_time_us, pcb->timing.running_time_us / 1000.0);
    fprintf(output, "Wait Time: %ld microseconds (%.3f ms)\n",
            pcb->timing.wait_time_us, pcb->timing.wait_time_us / 1000.0);
    fprintf(output, "Number of Schedules: %d\n", pcb->timing.scheduled_count);
    fprintf(output, "Page Faults: %d\n", pcb->page_faults);
    fprintf(output, "References: %ld\n", pcb->reference_counter);
    fprintf(output, "--------------------------------------\n");
}

/* Save process information */
int scissos_proc_save(ScisSosProcess *process, FILE *process_info)
{
    if (process == NULL || process_info == NULL)
    {
        fprintf(stderr, "Error: Invalid process or process_info.\n");
        return -1;
    }

    fprintf(process_info, "Process Name: %s\n", process->_pname);
    fprintf(process_info, "PID: %d\n", process->_PID);
    fprintf(process_info, "Size: %d\n", process->_psize);
    fprintf(process_info, "PCB Info:\n");
    scissos_print_pcb(process, process_info);

    return 0;
}

/* Run process with given PID */
/* Run process with given PID */
int scissos_proc_run(int pid, char *scheduler)
{
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d.\n", pid);
        return -1;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    if (pcb == NULL)
    {
        fprintf(stderr, "Error: Process with PID %d not found.\n", pid);
        return -1;
    }

    /* Record first schedule time */
    if (pcb->timing.scheduled_count == 0)
    {
        gettimeofday(&pcb->timing.first_schedule_time, NULL);
        pcb->timing.response_time_us = get_time_diff_us(
            pcb->timing.creation_time,
            pcb->timing.first_schedule_time);
    }

    struct timeval run_start;
    gettimeofday(&run_start, NULL);
    pcb->timing.scheduled_count++;

    fprintf(stdout, "\n[RUNNING] Process PID %d starting from PC = %d\n", pid, pcb->pc);

    int exec_instr = 0;
    int start_pc = pcb->pc;

    /* Execute instructions */
    while (pcb->pc < pcb->size)
    {
        ScisSosInst *instr = pcb->p_code[pcb->pc];

        /* Handle memory reference if memory manager is enabled */
        if (_memory_manager_enabled)
        {
            int logical_addr = instr->_addref;
            int frame = memory_get_frame(pid, logical_addr);

            /* memory_get_frame return contract:
               >=0  -> valid frame
               -1   -> error
               -2   -> memory manager disabled (treat as success)
            */
            if (frame == -1)
            {
                fprintf(stderr, "[ERROR] Memory access failed for PID %d at address %d\n",
                        pid, logical_addr);
                pcb->ps_state = PS_BLK;
                break;
            }
            else if (frame == -2)
            {
                /* Memory manager disabled — treat as successful access.
                   No frame to update; continue execution. */
            }
            else
            {
                /* frame >= 0 -> successful access; nothing more to do here */
            }
        }

        /* Long system call -> block */
        if (instr->_syscall == INS_LNG)
        {
            fprintf(stdout, "[BLOCKED] Process PID %d on instruction %d (Long Syscall)\n",
                    pid, instr->_inum);
            pcb->pc++;
            pcb->ps_state = PS_BLK;
            exec_instr++;
            break;
        }

        /* Short system call -> continue */
        pcb->pc++;
        exec_instr++;

        /* Time quantum exhaustion */
        if (exec_instr >= pcb->p_timeslice)
        {
            fprintf(stdout, "[TIME SLICE EXHAUSTED] Process PID %d after %d instructions\n",
                    pid, exec_instr);
            pcb->ps_state = PS_RDY;
            break;
        }
    }

    /* Check for completion */
    if (pcb->pc >= pcb->size)
    {
        fprintf(stdout, "[COMPLETED] Process PID %d completed\n", pid);
        pcb->ps_state = PS_DEAD;
        gettimeofday(&pcb->timing.completion_time, NULL);
        pcb->timing.running_time_us = get_time_diff_us(
            pcb->timing.first_schedule_time,
            pcb->timing.completion_time);
    }

    /* Update running time */
    struct timeval run_end;
    gettimeofday(&run_end, NULL);
    long run_time = get_time_diff_us(run_start, run_end);
    pcb->timing.running_time_us += run_time;

    fprintf(stdout, "[STATUS] Process PID %d moved from PC = %d to PC = %d, State = %d\n",
            pid, start_pc, pcb->pc, pcb->ps_state);

    /* Recursive scheduler call */
    scissos_call_scheduler(scheduler);

    return 0;
}

/* Delete process */
void scissos_proc_delete(int pid)
{
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d.\n", pid);
        return;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    if (pcb == NULL)
    {
        return;
    }

    /* Free pages from memory */
    if (_memory_manager_enabled && _physical_memory != NULL)
    {
        for (int i = 0; i < _num_frames; i++)
        {
            if (_physical_memory[i].pid == pid)
            {
                _physical_memory[i].pid = EMPTY;
                _physical_memory[i].page_number = EMPTY;
                _physical_memory[i].flags = 0;
            }
        }
    }

    /* Free address sequence */
    if (pcb->address_sequence != NULL)
    {
        free(pcb->address_sequence);
        pcb->address_sequence = NULL;
    }

    /* Free code memory */
    if (pcb->p_code)
    {
        for (int i = 0; i < pcb->size; i++)
        {
            if (pcb->p_code[i])
            {
                free(pcb->p_code[i]);
            }
        }
        free(pcb->p_code);
    }

    free(pcb);
    _proctable[pid - 1] = NULL;

    fprintf(stdout, "Process PID %d deleted from system\n", pid);
}
