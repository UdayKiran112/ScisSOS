#ifndef SCISSOS_H
#define SCISSOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

/**** Constants for Memory Management *********************************/
#define PAGESIZE 1024  /* Size of a Page of memory */
#define NUMFRAMES 64   /* No of Frames in Physical Memory */
#define REFWINDOW 1024 /* Reference window */

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
    int page_loaded[MAXPGES];         /* Pages currently loaded */
    int num_mem_pages;                /* Number of pages in memory */
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

/* Time strcutture*/
typedef struct
{
    struct timeval create_time;          /* Time when the process is created */
    struct timeval first_scheduled_time; /* Time when the process is first scheduled */
    struct timeval terminate_time;       /* Time when the process is terminated */
    struct timeval wait_time;            /* Total time spent for waiting */
    struct timeval last_ready_time;      /* Last time when moved to ready queue */
    int response_flag;
} Time;

/*Frame structure*/
typedef struct
{
    int page_number;   /* Page number loaded in this frame */
    int pid;           /* PID of the process using this frame */
    int dirty;         /* Dirty bit */
    int use;           /* Use bit */
    int reference_cnt; /* Reference count */
} Frame;

/** Data structures used by the OS to do its management actions **/
extern ScisSosPCB *_proctable[MAXPROC]; /* Process Table */
extern int _readyQ[MAXPROC];            /* Ready Queue */
extern int _blockQ[MAXPROC];            /* Wait Queue */
extern int _currentPID;                 /* Current running process PID */
extern struct timeval base_time;        /* Base time for scheduling */
extern Time process_times[MAXPROC];     /* Time tracking for processes */
extern Frame frame_table[NUMFRAMES];    /* Frame Table --> Physical Memory */

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

/* Memory functions*/
void memory_initialise(void);                        /* Initialise memory management */
int memory_page_fault_handler(int pid, int page);    /* Handle page faults */
int memory_get_page(int pid, int address);           /* Get page number for address */
int memory_fifo(void);                               /* FIFO page replacement */
int memory_lru(void);                                /* LRU page replacement */
void print_memory_stats(void);                       /* Print memory stats */
int *memory_gen_addrefstrings(int size, int m_type); /* Generate address reference strings */

/* Time calculation functions*/
struct timeval difference_times(struct timeval start, struct timeval end); /* Calculate difference between two timeval structs*/
struct timeval add_times(struct timeval t1, struct timeval t2);            /* Add two timeval structs */
void print_time(struct timeval t);                                         /* Print timeval struct */
double timeval_to_seconds(struct timeval t);                               /* Convert timeval to seconds */

#endif