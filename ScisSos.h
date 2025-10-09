#ifndef SCISSOS_H
#define SCISSOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****
 *  Constants defining OS parameters
 ****/
#define MAXPROC 1000  /* Max number of processes */
#define MAXUSRS 10    /* Max number of users */
#define DEFPRIO 20    /* Default priority for process */
#define EMPTY -100    /* Unfilled entries */
#define MAXPGES 10    /* Max number of pages/process */
#define DEFTS 6239    /* Default time slice */
#define REG_THR 0.02  /* Normal process: 2% long calls */
#define CMP_THR 0.001 /* Compute Intensive: 0.1% */
#define IOE_THR 0.2   /* IO Intensive: 20% long calls */

/**** Constants for Process States ************************************/
#define PS_NEW 0
#define PS_RDY 1
#define PS_RUN 2
#define PS_BLK 3
#define PS_SRDY 4
#define PS_SBLK 5
#define PS_DEAD 6

/**** Constants for Process Types *************************************/
#define PT_REG 0   /* Regular Process */
#define PT_CMP 1   /* Compute Intensive Process */
#define PT_IOE 2   /* IO Intensive Process */
#define MT_GOOD 3  /* Structured Memory Usage */
#define MT_BAD 4   /* Unstructured Memory Usage */
#define MT_UGLY 5  /* Spaghetti Code! */
#define INS_LNG 10 /* Long instruction */
#define INS_SHR 20 /* Short instruction */

typedef int ScisSosPGTable[2];

/** Instruction in a process; process is a sequence of instructions **/
typedef struct
{
    int _inum;    /* Instruction Number */
    int _syscall; /* System call type: long/short */
    int _addref;  /* Memory address reference */
} ScisSosInst;

/** Process Control Block structure **/
typedef struct
{
    int pid;                          /* Process ID 1 to MAXPROC */
    int uid;                          /* User ID 1 to MAXUSRS */
    int size;                         /* Size specified by users */
    int priority_value;               /* Priority value */
    int ps_state;                     /* Process State */
    int p_type;                       /* Process Type (See above) */
    int m_type;                       /* Memory behaviour */
    int pc;                           /* Program Counter */
    ScisSosInst **p_code;             /* Pointer to exectable code */
    ScisSosPGTable pg_table[MAXPGES]; /* Page Table Information */
    int p_timeslice;                  /* Current Time-Slice */
} ScisSosPCB;

/** Process Structure **/
typedef struct
{
    char _pname[80];     /* Name of the process */
    int _PID;            /* PID */
    int _psize;          /* Size of the process */
    ScisSosPCB *_pcb;    /* Pointer to its PCB */
    ScisSosInst **_CODE; /* Pointer to its code */
} ScisSosProcess;

/** Data structures used by the OS to do its management actions **/
extern ScisSosPCB *_proctable[MAXPROC]; /* Process Table */
extern int _readyQ[MAXPROC];            /* Ready Queue */
extern int _blockQ[MAXPROC];            /* Wait Queue */
extern int _currentPID;                 /* Current running process PID */

/** Process-related functions found in process.c file **/
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type); /* Create a new process */
int scissos_proc_save(ScisSosProcess *process, FILE *process_info);                          /* Save process info to file */
void scissos_print_pcb(ScisSosProcess *process, FILE *pcb_info);                             /* Print PCB info */
int scissos_proc_run(int pid, char *scheduler);                                              /* Run the process with given PID */
void scissos_proc_delete(int pid);                                                           /* Delete the process with given PID */

/** OS-related functions found in os.c file **/
void scissos_initialise(void);                /* Initialise the OS */
void scissos_call_scheduler(char *scheduler); /* Call the scheduler */
void scisos_update_queues(void);              /* Update the ready and block queues */
int scissos_count_ready_processes(void);      /* Count ready processes */
void scissos_unblock_process(void);           /* Unblock processes */
int scisos_active_processes(void);            /* Check for active processes */

#endif