#ifndef SCHEDULING_ALGO_H
#define SCHEDULING_ALGO_H

#include "ScisSos.h"

// All Scheduling Algos
int scissos_schedule_fcfs(int *readyQ, int qsize);     /* First Come First Serve */
int scissos_schedule_sjf(int *readyQ, int qsize);      /* Shortest Job First */
int scissos_schedule_priority(int *readyQ, int qsize); /* Priority */
int scissos_schedule_rr(int *readyQ, int qsize);       /* Round Robin */

#endif