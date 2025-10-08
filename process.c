#include "ScisSos.h"

static int pid_counter = 1; // Global PID counter

// Generate code for a process based on its type
ScisSosInst **scissos_generate_code(int size, int p_type)
{
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

        // Generate random memory address reference
        code[i]->_addref = rand() % 1000;
    }

    return code;
}

// Create and initialise a PCB
void scissos_create_pcb(ScisSosProcess *process, int pid, int uid, int size, int priority, int p_type, ScisSosInst **code)
{
    process->_pcb = (ScisSosPCB *)malloc(sizeof(ScisSosPCB));
    process->_pcb->pid = pid;
    process->_pcb->uid = uid;
    process->_pcb->size = size;
    process->_pcb->priority_value = priority;
    process->_pcb->ps_state = PS_NEW; // Initial state is New
    process->_pcb->p_type = p_type;
    process->_pcb->m_type = MT_GOOD;
    process->_pcb->pc = 0; // Program counter starts at 0
    process->_pcb->p_code = code;
    process->_pcb->p_timeslice = DEFTS; // Initial time slice

    // Page table initialisation
    for (int i = 0; i < MAXPGES; i++)
    {
        process->_pcb->pg_table[i][0] = i;
        process->_pcb->pg_table[i][1] = EMPTY;
    }
}

// Create a new process and return its pointer
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type)
{
    int pid, uid;

    pid = pid_counter++;
    uid = rand() % MAXUSRS + 1; // Random UID between 1 and MAXUSRS

    // Generate code for process
    ScisSosInst **code = scissos_generate_code(size, p_type);

    // Check size validity
    if (size < 0)
    {
        fprintf(stderr, "Error: Invalid size %d. Must be non-negative.\n", size);
        return NULL; // Invalid size
    }

    // Memory allocation
    ScisSosProcess *new_process = (ScisSosProcess *)malloc(sizeof(ScisSosProcess));

    snprintf(new_process->_pname, sizeof(new_process->_pname), "%s", process_name);
    new_process->_PID = pid;
    new_process->_psize = size;
    new_process->_CODE = code;

    // PCB creation and initialisation
    scissos_create_pcb(new_process, pid, uid, size, priority, p_type, new_process->_CODE);

    // Add process to process table
    _proctable[pid - 1] = new_process->_pcb;

    // set process state to ready
    new_process->_pcb->ps_state = PS_RDY;

    fprintf(stdout, "Process created: %s, PID: %d, UID: %d\n, Priority: %d\n, Type: %d\n", process_name, pid, uid, priority, p_type);

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

    fprintf(pcb_info, "PID: %d\n", process->_pcb->pid);
    fprintf(pcb_info, "UID: %d\n", process->_pcb->uid);
    fprintf(pcb_info, "Size: %d\n", process->_pcb->size);
    fprintf(pcb_info, "Priority: %d\n", process->_pcb->priority_value);
    fprintf(pcb_info, "State: %d\n", process->_pcb->ps_state);
    fprintf(pcb_info, "Type: %d\n", process->_pcb->p_type);
    fprintf(pcb_info, "Memory Type: %d\n", process->_pcb->m_type);
    fprintf(pcb_info, "Program Counter: %d\n", process->_pcb->pc);
    fprintf(pcb_info, "Time Slice: %d\n", process->_pcb->p_timeslice);
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
int scissos_proc_run(int pid)
{
    // get pcb from process table
    ScisSosPCB *pcb = _proctable[pid - 1];

    if (pcb == NULL)
    {
        fprintf(stderr, "Error: Process not found.\n");
        return -1;
    }

    fprintf(stdout, "[RUNNING] Process PID : %d starting from PC = %d\n", pid, pcb->pc);

    int exec_instr = 0; /* Number of instructions executed */
    int start_pc = pcb->pc;

    // Execute instructions
    while (pcb->pc < pcb->size)
    {
        ScisSosInst *instr = pcb->p_code[pcb->pc];

        // long system call -> block the process
        if (instr->_syscall == INS_LNG)
        {
            fprintf(stdout, "[BLOCKED] Process PID : %d on instruction %d (Long Syscall)\n", pid, instr->_inum);
            pcb->pc++; // Move to next instruction
            pcb->ps_state = PS_BLK;
            exec_instr++;
            break;
        }

        // short system call -> continue execution
        fprintf(stdout, "[EXECUTING] Process PID : %d executing instruction %d (Short Syscall)\n", pid, instr->_inum);
        pcb->pc++; // Move to next instruction
        exec_instr++;

        // time quantum exhaustion case
        if (exec_instr >= pcb->p_timeslice)
        {
            fprintf(stdout, "[TIME SLICE EXHAUSTED] Process PID : %d after executing %d instructions\n", pid, exec_instr);
            pcb->ps_state = PS_RDY;
            break;
        }
    }

    // check for process completion
    if (pcb->pc >= pcb->size)
    {
        fprintf(stdout, "[COMPLETED] Process PID : %d completed\n", pid);
        pcb->ps_state = PS_DEAD;
    }

    fprintf(stdout, "[STATUS] Process PID : %d moved from PC = %d to PC = %d, State = %d\n", pid, start_pc, pcb->pc, pcb->ps_state);

    // call scheduler
    scissos_call_scheduler();

    return 0;
}

// Delete the process with the given PID
void scissos_proc_delete(int pid)
{
    // Error handling
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d.\n", pid); // Invalid PID
        return;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    if (pcb == NULL)
    {
        fprintf(stderr, "Error: Process with PID %d not found.\n", pid); // Process not found
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
}