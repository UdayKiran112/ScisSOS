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
#define MAXPGES 100   /* Max number of pages/process */
#define DEFTS 6239    /* Default time slice */
#define REG_THR 0.02  /* Normal process: 2% long calls */
#define CMP_THR 0.001 /* Compute Intensive: 0.1% */
#define IOE_THR 0.2   /* IO Intensive: 20% long calls */

/**** Memory Management Constants ************************************/
#define PAGESIZE 4096         /* Size of each page in bytes */
#define NUMFRAMES 64          /* Default number of frames in physical memory */
#define REFERENCE_WINDOW 1024 /* Reference window for page replacement */

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

/**** Memory Manager Flags ********************************************/
#define DIRTY_BIT 0x01 /* Page has been referenced recently */
#define USE_BIT 0x02   /* Page has been modified */

typedef int ScisSosPGTable[2];

/** Timing Information Structure **/
typedef struct
{
    struct timeval creation_time;       /* Time when process was created */
    struct timeval first_schedule_time; /* Time when first scheduled */
    struct timeval completion_time;     /* Time when process completed */
    long response_time_us;              /* Response time in microseconds */
    long running_time_us;               /* Total running time in microseconds */
    long wait_time_us;                  /* Total wait time in microseconds */
    int scheduled_count;                /* Number of times process was scheduled */
} TimingInfo;

/** Instruction in a process; process is a sequence of instructions **/
typedef struct
{
    int _inum;    /* Instruction Number */
    int _syscall; /* System call type: long/short */
    int _addref;  /* Memory address reference */
} ScisSosInst;

/** Frame Structure for Physical Memory **/
typedef struct
{
    int page_number;     /* Page number currently in this frame */
    int pid;             /* PID of process owning this page */
    unsigned char flags; /* DIRTY_BIT | USE_BIT */
    long last_reference; /* Counter for LRU */
    int load_time;       /* For FIFO */
} FrameEntry;

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
    ScisSosInst **p_code;             /* Pointer to executable code */
    int *address_sequence;            /* Memory address reference sequence from generator */
    int num_pages;                    /* Number of pages needed for this process */
    ScisSosPGTable pg_table[MAXPGES]; /* Page Table: [page_num][frame_num] */
    int p_timeslice;                  /* Current Time-Slice */
    TimingInfo timing;                /* Timing information */
    int page_faults;                  /* Count of page faults */
    long reference_counter;           /* Reference counter for this process */
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

/** Memory Manager Statistics **/
typedef struct
{
    int total_page_faults;
    int total_page_replacements;
    int total_page_loads;
} MemoryStats;

/** Data structures used by the OS to do its management actions **/
extern ScisSosPCB *_proctable[MAXPROC]; /* Process Table */
extern int _readyQ[MAXPROC];            /* Ready Queue */
extern int _blockQ[MAXPROC];            /* Wait Queue */
extern int _currentPID;                 /* Current running process PID */
extern FrameEntry *_physical_memory;    /* Physical memory frames */
extern int _num_frames;                 /* Number of frames in physical memory */
extern long _global_reference_counter;  /* Global reference counter */
extern MemoryStats _mem_stats;          /* Memory statistics */
extern int _memory_manager_enabled;     /* Flag to enable/disable memory manager */

/** Process-related functions found in process.c file **/
ScisSosProcess *scissos_proc_create(char *process_name, int size, int priority, int p_type);
int scissos_proc_save(ScisSosProcess *process, FILE *process_info);
void scissos_print_pcb(ScisSosProcess *process, FILE *pcb_info);
int scissos_proc_run(int pid, char *scheduler);
void scissos_proc_delete(int pid);
void scissos_print_timing_info(ScisSosPCB *pcb, FILE *output);

/** OS-related functions found in os.c file **/
void scissos_initialise(void);
void scissos_call_scheduler(char *scheduler);
void scisos_update_queues(void);
int scissos_count_ready_processes(void);
void scissos_unblock_process(void);
int scisos_active_processes(void);

/** Memory management functions found in memory.c file **/
void memory_manager_init(int num_frames);
void memory_manager_cleanup(void);
int memory_handle_page_fault(int pid, int page_number);
int memory_load_page(int pid, int page_number);
int memory_get_frame(int pid, int logical_address);
void memory_update_reference(int pid, int page_number);
void memory_print_stats(FILE *output);

/** Page replacement algorithms **/
int page_replace_fifo(int pid, int page_number);
int page_replace_lru(int pid, int page_number);
int page_replace_clock(int pid, int page_number);

/** Utility functions **/
long get_time_diff_us(struct timeval start, struct timeval end);

/** Memory address generator function from library **/
int *memory_gen_addrefstrings(int size, int mtype);

#endif