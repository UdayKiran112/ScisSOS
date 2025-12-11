/* memory_gen.c
   Robust implementation of memory_gen_addrefstrings(size, mtype)
   - Caller must free returned int*.
   - Avoids division/modulo by zero.
   - Provides simple locality models: strong, medium, weak, working-set.
   - Includes WS_RATIO table similar to the library's expected data.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

/* Fallback macros if not provided by project headers */
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif
#ifndef MAXPGES
#define MAXPGES 64
#endif

/* A small working-set ratio table (must be non-zero) */
int WS_RATIO[] = {8, 6, 4, 2, 1, 1, 1, 1};

/* Helper: clamp an integer into [0, max-1] */
static inline int clamp_idx(long v, int max)
{
    if (max <= 0)
        return 0;
    long m = v % max;
    if (m < 0)
        m += max;
    return (int)m;
}

/* Helper: produce an address within page*pagesize + offset */
static inline int make_addr(int page, int page_size)
{
    /* offset within page: keep small to avoid hitting huge addresses */
    int offset = rand() % page_size;
    long addr = (long)page * (long)page_size + offset;
    return (int)(addr & 0x7fffffff);
}

/* Simple working-set generator: returns working set size or -1 on error */
static int compute_workingset(int pages, int mtype)
{
    /* Use WS_RATIO[mtype] if available, else fallback */
    int mcount = sizeof(WS_RATIO) / sizeof(WS_RATIO[0]);
    int ratio = (mtype >= 0 && mtype < mcount) ? WS_RATIO[mtype] : 2;
    if (ratio <= 0)
        ratio = 2;
    int ws = pages / ratio;
    if (ws < 1)
        ws = 1;
    return ws;
}

/* Primary function (signature expected by ScisSOS) */
int *memory_gen_addrefstrings(int size, int mtype)
{
    if (size <= 0)
    {
        fprintf(stderr, "memory_gen_addrefstrings: invalid size=%d\n", size);
        return NULL;
    }

    int page_size = (PAGESIZE > 0) ? PAGESIZE : 4096;
    int max_pges = (MAXPGES > 0) ? MAXPGES : 64;

    long total_addr_space = (long)page_size * (long)max_pges;
    if (total_addr_space <= 0)
    {
        page_size = 4096;
        max_pges = 64;
        total_addr_space = (long)page_size * (long)max_pges;
    }

    /* number of pages the process will span (at least 1) */
    int pages = (int)((size + page_size - 1) / page_size);
    if (pages <= 0)
        pages = 1;
    if (pages > max_pges)
        pages = max_pges;

    int *refs = (int *)malloc(sizeof(int) * size);
    if (!refs)
    {
        fprintf(stderr, "memory_gen_addrefstrings: malloc failed\n");
        return NULL;
    }

    /* Seed RNG once (safe if called multiple times) */
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned)time(NULL) ^ (unsigned)(uintptr_t)&refs);
        seeded = 1;
    }

    /* Behavior by mtype:
       - 0: compute-heavy process -> many long instructions concentrated (strong locality)
       - 1: io-bound -> bursts / clustered locality
       - 2: regular -> medium locality
       - >=3: random / mixed -> weak locality / near-uniform
       We also support a simple working-set mode that biases addresses to a working-set window.
    */

    int working_set = compute_workingset(pages, mtype);
    if (working_set <= 0)
        working_set = 1;
    /* choose a base page for the working set */
    int ws_base = rand() % max_pges;
    /* ensure ws_base such that ws fits */
    if (working_set > max_pges)
        working_set = max_pges;

    for (int i = 0; i < size; ++i)
    {
        unsigned long addr = 0;
        if (mtype == 0)
        {
            /* strong locality: pick from a tight cluster around ws_base */
            int center = ws_base % max_pges;
            int offset_page = (rand() % 3) - 1; /* -1, 0, +1 */
            int page = clamp_idx(center + offset_page, max_pges);
            addr = (unsigned long)page * page_size + (rand() % page_size);
        }
        else if (mtype == 1)
        {
            /* IO-like bursts: occasional jumps, mostly clustered */
            if ((rand() % 10) < 7)
            {
                /* within working set */
                int page = ws_base + (rand() % working_set);
                page = clamp_idx(page, max_pges);
                addr = (unsigned long)page * page_size + (rand() % page_size);
            }
            else
            {
                /* big jump */
                int page = rand() % max_pges;
                addr = (unsigned long)page * page_size + (rand() % page_size);
            }
        }
        else if (mtype == 2)
        {
            /* medium locality: sliding window across pages */
            int window = working_set;
            if (window < 2)
                window = 2;
            int base = (ws_base + (rand() % (max_pges - window + 1 + (max_pges < window)))) % max_pges;
            int page = (base + (rand() % window)) % max_pges;
            addr = (unsigned long)page * page_size + (rand() % page_size);
        }
        else
        {
            /* mtype >= 3 : near-uniform random across whole address space */
            unsigned long r = (unsigned long)rand();
            unsigned long max_addr = (unsigned long)(total_addr_space - 1);
            if (max_addr == 0)
                max_addr = (unsigned long)page_size * 10 - 1;
            addr = (unsigned long)((r * max_addr) / (unsigned long)RAND_MAX);
        }

        refs[i] = (int)(addr & 0x7fffffff);
    }

    return refs;
}
