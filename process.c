#include "ScisSos.h"
#include "ScisosMem.h" 

static int pid_counter = 1;

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
        double rand_val = (double)rand() / RAND_MAX;
        if (rand_val < long_call_prob)
        {
            code[i]->_syscall = INS_LNG;
        }
        else
        {
            code[i]->_syscall = INS_SHR;
        }

        /* Memory address will be set from the generated sequence */
        code[i]->_addref = 0; /* Will be updated from address_sequence */
    }

    return code;
}

/* Create and initialize a PCB */
void scissos_create_pcb(ScisSosProcess *process, int pid, int uid, int size,
                        int priority, int p_type, int m_type, ScisSosInst **code)
{
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

    /* Calculate number of pages needed */
    /* Assuming each instruction represents one memory reference */
    process->_pcb->num_pages = (size * PAGESIZE + PAGESIZE - 1) / PAGESIZE;
    if (process->_pcb->num_pages > MAXPGES)
    {
        process->_pcb->num_pages = MAXPGES;
    }

    /* Generate memory address reference sequence using ScisSosMem library */
    process->_pcb->address_sequence = memory_gen_addrefstrings(size, m_type);

    if (process->_pcb->address_sequence == NULL)
    {
        fprintf(stderr, "Warning: Failed to generate address sequence for PID %d\n", pid);
    }
    else
    {
        /* Copy addresses to instruction address references */
        for (int i = 0; i < size; i++)
        {
            code[i]->_addref = process->_pcb->address_sequence[i];
        }
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
        process->_pcb->pg_table[i][0] = i;     /* Page number */
        process->_pcb->pg_table[i][1] = EMPTY; /* Frame number (initially not loaded) */
    }
}

/* Create a new process */
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type)
{
    // Check size validity
    if (size <= 0)
    {
        fprintf(stderr, "Error: Invalid size %d. Must be positive.\n", size);
        return NULL;
    }

    // Check if we have space in process table
    if (pid_counter > MAXPROC)
    {
        fprintf(stderr, "Error: Process table full. Cannot create more processes.\n");
        return NULL;
    }

    int pid = pid_counter++;
    int uid = rand() % MAXUSRS + 1;

    /* Default memory type - will be set by caller if needed */
    int m_type = MT_GOOD;

    // Generate code for process
    ScisSosInst **code = scissos_generate_code(size, p_type);
    if (!code)
    {
        fprintf(stderr, "Error: Failed to generate code for process.\n");
        return NULL;
    }

    // Memory allocation for process structure
    ScisSosProcess *new_process = (ScisSosProcess *)malloc(sizeof(ScisSosProcess));
    if (!new_process)
    {
        fprintf(stderr, "Error: Memory allocation failed for process structure.\n");
        // Free the code
        for (int i = 0; i < size; i++)
        {
            free(code[i]);
        }
        free(code);
        return NULL;
    }

    snprintf(new_process->_pname, sizeof(new_process->_pname), "%s", process_name);
    new_process->_PID = pid;
    new_process->_psize = size;
    new_process->_CODE = code;

    scissos_create_pcb(new_process, pid, uid, size, priority, p_type, m_type, new_process->_CODE);
    _proctable[pid - 1] = new_process->_pcb;

    /* If memory manager is enabled, load first page */
    if (_memory_manager_enabled)
    {
        memory_load_page(pid, 0);
    }

    new_process->_pcb->ps_state = PS_RDY;

    fprintf(stdout, "Process created: %s, PID: %d, UID: %d, Priority: %d, Type: %d, MemType: %d\n",
            process_name, pid, uid, priority, p_type, m_type);

    return new_process;
}

/* Print PCB information */
void scissos_print_pcb(ScisSosProcess *process, FILE *pcb_info)
{
    // Error handling
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
int scissos_proc_run(int pid, char *scheduler)
{
    // Validate PID
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d.\n", pid);
        return -1;
    }

    // get pcb from process table
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

            /* Get frame (will handle page fault if needed) */
            int frame = memory_get_frame(pid, logical_addr);
            if (frame < 0)
            {
                fprintf(stderr, "[ERROR] Memory access failed for PID %d at address %d\n",
                        pid, logical_addr);
                pcb->ps_state = PS_BLK;
                break;
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
    // Error handling
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d.\n", pid);
        return;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    if (pcb == NULL)
    {
        // Process already deleted or doesn't exist
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

    // free pcb memory
    free(pcb);

    // Remove from process table
    _proctable[pid - 1] = NULL;

    fprintf(stdout, "Process PID %d deleted from system\n", pid);
}