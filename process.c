#include "ScisSos.h"

static int pid_counter = 1; // Global PID counter

// Generate code for a process based on its type
ScisSosInst **scissos_generate_code(int size, int p_type, int *reference_addrs)
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
            // Free previously allocated instructions
            for (int j = 0; j < i; j++)
            {
                free(code[j]);
            }
            free(code);
            return NULL;
        }

        code[i]->_inum = i;

        // Determine if this is a long or short system call
        double rand_val = (double)rand() / RAND_MAX;
        if (rand_val < long_call_prob)
        {
            code[i]->_syscall = INS_LNG;
        }
        else
        {
            code[i]->_syscall = INS_SHR;
        }

        // Set address references for this instruction
        code[i]->_addref = reference_addrs[i];
    }

    return code;
}

// Create and initialise a PCB
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
    process->_pcb->ps_state = PS_NEW; // Initial state is New
    process->_pcb->p_type = p_type;
    process->_pcb->m_type = m_type;
    process->_pcb->pc = 0; // Program counter starts at 0
    process->_pcb->p_code = code;
    process->_pcb->p_timeslice = DEFTS; // Initial time slice
    process->_pcb->num_mem_pages = 0;

    // Page table initialisation
    for (int i = 0; i < MAXPGES; i++)
    {
        process->_pcb->pg_table[i][0] = i;
        process->_pcb->pg_table[i][1] = EMPTY;
        process->_pcb->page_loaded[i] = 0;
    }
}

// Create a new process and return its pointer
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type, int m_type)
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
    int uid = rand() % MAXUSRS + 1; // Random UID between 1 and MAXUSRS

    // Memory allocation for process structure
    ScisSosProcess *new_process = (ScisSosProcess *)malloc(sizeof(ScisSosProcess));
    if (!new_process)
    {
        fprintf(stderr, "Error: Memory allocation failed for process structure.\n");
        return NULL;
    }

    snprintf(new_process->_pname, sizeof(new_process->_pname), "%s", process_name);
    new_process->_PID = pid;
    new_process->_psize = size;

    // Generate memory address references FIRST
    int *reference_addr = memory_gen_addrefstrings(size, m_type);

    fprintf(stdout, "=== Creating Process: %s ===\n", process_name);


    if (reference_addr == NULL) // FIX: Check for NULL, not 0
    {
        fprintf(stderr, "Error: Memory allocation failed for memory address references.\n");
        free(new_process);
        return NULL; // FIX: Return NULL instead of just returning
    }

    // Generate code for process
    ScisSosInst **code = scissos_generate_code(size, p_type, reference_addr);
    if (!code)
    {
        fprintf(stderr, "Error: Failed to generate code for process.\n");
        free(reference_addr);
        free(new_process);
        return NULL;
    }

    // Now set the CODE pointer
    new_process->_CODE = code;

    // PCB creation and initialisation - pass the actual code now
    scissos_create_pcb(new_process, pid, uid, size, priority, p_type, m_type, code);

    // Free the reference addresses - no longer needed
    free(reference_addr);

    // Add process to process table
    _proctable[pid - 1] = new_process->_pcb;

    // timings initialisation
    gettimeofday(&process_times[pid - 1].create_time, NULL);
    process_times[pid - 1].response_flag = 0;
    process_times[pid - 1].wait_time.tv_sec = 0;
    process_times[pid - 1].wait_time.tv_usec = 0;
    process_times[pid - 1].last_ready_time.tv_sec = 0;
    process_times[pid - 1].last_ready_time.tv_usec = 0;

    fprintf(stdout, "Process created: %s, PID: %d, UID: %d, Priority: %d, Type: %d, Memory Type: %d\n",
            process_name, pid, uid, priority, p_type, m_type);
    fprintf(stdout, "Timeslice: %d\n", new_process->_pcb->p_timeslice);

    // if memory is enabled, load the first page
    if (_memory_enabled)
    {
        int first = 0;
        int frame = memory_get_page(pid - 1, first);
        if (frame != -1)
        {
            new_process->_pcb->pg_table[first][1] = frame;
            new_process->_pcb->page_loaded[first] = 1;
            new_process->_pcb->num_mem_pages++;
            fprintf(stdout, "First page loaded into frame %d\n", frame);
        }
    }

    // set process state to ready
    new_process->_pcb->ps_state = PS_RDY;
    gettimeofday(&process_times[pid - 1].last_ready_time, NULL);

    fprintf(stdout, "Process PID %d moved from NEW to READY state\n", pid);

    return new_process;
}

// print the PCB of a process
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

    fprintf(pcb_info, "----------------------------------------\n");
    fprintf(pcb_info, "Process Name: %s\n", process->_pname);
    fprintf(pcb_info, "PID: %d\n", process->_pcb->pid);
    fprintf(pcb_info, "UID: %d\n", process->_pcb->uid);
    fprintf(pcb_info, "Size: %d\n", process->_pcb->size);
    fprintf(pcb_info, "Priority: %d\n", process->_pcb->priority_value);
    fprintf(pcb_info, "State: %s (%d)\n", state_names[process->_pcb->ps_state], process->_pcb->ps_state);
    fprintf(pcb_info, "Type: %s (%d)\n", type_names[process->_pcb->p_type], process->_pcb->p_type);
    fprintf(pcb_info, "Memory Type: %d\n", process->_pcb->m_type);
    fprintf(pcb_info, "Program Counter: %d\n", process->_pcb->pc);
    fprintf(pcb_info, "Time Slice: %d\n", process->_pcb->p_timeslice);
    fprintf(pcb_info, "No. of loaded pages: %d\n", process->_pcb->num_mem_pages);

    // page table printing -> print only few pages(5)
    fprintf(pcb_info, "\nPage Table:\n");
    fprintf(pcb_info, "Page | Frame | Loaded\n");
    fprintf(pcb_info, "-----+-------+-------\n");
    for (int i = 0; i < 5; i++)
    {
        fprintf(pcb_info, " %2d  |  %3d  |   %s\n",
                process->_pcb->pg_table[i][0],
                process->_pcb->pg_table[i][1],
                process->_pcb->page_loaded[i] ? "YES" : "NO");
    }

    fprintf(pcb_info, "----------------------------------------\n");
}

// save the process information to a file
int scissos_proc_save(ScisSosProcess *process, FILE *process_info)
{
    // Error handling
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

// run the process with the given PID
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

    fprintf(stdout, "\n[RUNNING] Process PID %d starting from PC = %d\n", pid, pcb->pc);

    struct timeval start_time, end_time;

    int exec_instr = 0; /* Number of instructions executed */
    gettimeofday(&start_time, NULL);
    int start_pc = pcb->pc;

    // Execute instructions
    while (pcb->pc < pcb->size)
    {
        ScisSosInst *instr = pcb->p_code[pcb->pc];
        int adrs = instr->_addref;
        int page_number = adrs / PAGESIZE;

        if (_memory_enabled)
        {
            if (!pcb->page_loaded[page_number])
            {
                fprintf(stdout, "[PC = %d] PAGE FAULT OCCURED\n", pcb->pc);
                page_faults++;

                int frame = memory_page_fault_handler(pid, page_number);

                if (frame != -1)
                {
                    pcb->pg_table[page_number][1] = frame;
                    pcb->page_loaded[page_number] = 1;
                    pcb->num_mem_pages++;
                    fprintf(stdout, "[PC = %d] PAGE FAULT HANDLED, FRAME = %d\n", pcb->pc, frame);
                }
            }
        }

        // long system call -> block the process
        if (instr->_syscall == INS_LNG)
        {
            fprintf(stdout, "[BLOCKED] Process PID %d on instruction %d (Long Syscall)\n",
                    pid, instr->_inum);
            pcb->pc++; // Move to next instruction
            pcb->ps_state = PS_BLK;
            exec_instr++;
            break;
        }

        // continue execution
        // fprintf(stdout, "[EXECUTING] Process PID %d executing instruction %d (Short Syscall)\n",
        //         pid, instr->_inum);
        pcb->pc++; // Move to next instruction
        exec_instr++;

        gettimeofday(&end_time, NULL);

        // time quantum exhaustion case
        if (exec_instr >= pcb->p_timeslice)
        {
            fprintf(stdout, "[TIME SLICE EXHAUSTED] Process PID %d after executing %d instructions\n",
                    pid, exec_instr);
            pcb->ps_state = PS_RDY;
            break;
        }
    }

    // check for process completion
    if (pcb->pc >= pcb->size)
    {
        fprintf(stdout, "[COMPLETED] Process PID %d completed\n", pid);
        pcb->ps_state = PS_DEAD;
    }

    fprintf(stdout, "[STATUS] Process PID %d moved from PC = %d to PC = %d, State = %d\n",
            pid, start_pc, pcb->pc, pcb->ps_state);

    struct timeval time_taken = difference_times(end_time, start_time);

    fprintf(stdout, "[TIME TAKEN] Process PID %d took %lf seconds to execute %d instructions\n",
            pid, timeval_to_seconds(time_taken), exec_instr);

    if (pcb->ps_state == PS_BLK)
    {
        gettimeofday(&process_times[pid - 1].last_ready_time, NULL);
        fprintf(stdout, "[BLOCKED] Process PID %d moved to READY state\n", pid);
    }

    // call scheduler recursively
    scissos_call_scheduler(scheduler);

    return 0;
}

// Delete the process with the given PID
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

    // Free code memory
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