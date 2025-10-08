#include "ScisSos.h"

// Create a new process and return its pointer
ScisSosProcess *scissos_proc_create(char *process_name, int pid, int uid, int size)
{
    // Check PID validity
    if (pid < 0 || pid >= MAXPROC)
    {
        return NULL; // Invalid PID
    }

    // Check UID validity
    if (uid < 0 || uid >= MAXUSRS)
    {
        return NULL; // Invalid UID
    }
}

// save the process information to a file
int scissos_proc_save(ScisSosProcess *process, FILE *process_info)
{
}

// print the PCB of a process
void scissos_print_pcb(ScisSosProcess *process, FILE *pcb_info)
{
}

// run the process with the given PID
int scissos_proc_run(int pid)
{
}