#include "ScisSos.h"

FrameEntry *_physical_memory = NULL;
int _num_frames = NUMFRAMES;
long _global_reference_counter = 0;
MemoryStats _mem_stats = {0, 0, 0};
int _memory_manager_enabled = 1;
static int clock_hand = 0; /* For clock algorithm */

/* Initialize the memory manager */
void memory_manager_init(int num_frames)
{
    _num_frames = num_frames;

    _physical_memory = (FrameEntry *)malloc(_num_frames * sizeof(FrameEntry));
    if (!_physical_memory)
    {
        fprintf(stderr, "Error: Failed to allocate physical memory\n");
        exit(1);
    }

    /* Initialize all frames as empty */
    for (int i = 0; i < _num_frames; i++)
    {
        _physical_memory[i].page_number = EMPTY;
        _physical_memory[i].pid = EMPTY;
        _physical_memory[i].flags = 0;
        _physical_memory[i].last_reference = 0;
        _physical_memory[i].load_time = 0;
    }

    _global_reference_counter = 0;
    _mem_stats.total_page_faults = 0;
    _mem_stats.total_page_replacements = 0;
    _mem_stats.total_page_loads = 0;
    clock_hand = 0;

    fprintf(stdout, "[MEMORY] Memory manager initialized with %d frames (Page size: %d bytes)\n",
            _num_frames, PAGESIZE);
}

/* Cleanup memory manager */
void memory_manager_cleanup(void)
{
    if (_physical_memory)
    {
        free(_physical_memory);
        _physical_memory = NULL;
    }
}

/* Find a free frame in physical memory */
int find_free_frame(void)
{
    for (int i = 0; i < _num_frames; i++)
    {
        if (_physical_memory[i].pid == EMPTY)
        {
            return i;
        }
    }
    return EMPTY; /* No free frame */
}

/* Check if a page is already loaded in memory */
int is_page_loaded(int pid, int page_number)
{
    for (int i = 0; i < _num_frames; i++)
    {
        if (_physical_memory[i].pid == pid &&
            _physical_memory[i].page_number == page_number)
        {
            return i; /* Return frame number */
        }
    }
    return EMPTY; /* Page not loaded */
}

/* FIFO Page Replacement Algorithm */
int page_replace_fifo(int pid, int page_number)
{
    int oldest_frame = 0;
    int oldest_time = _physical_memory[0].load_time;

    /* Find the frame with the oldest load time */
    for (int i = 1; i < _num_frames; i++)
    {
        if (_physical_memory[i].load_time < oldest_time)
        {
            oldest_time = _physical_memory[i].load_time;
            oldest_frame = i;
        }
    }

    fprintf(stdout, "[FIFO] Replacing page %d of PID %d in frame %d\n",
            _physical_memory[oldest_frame].page_number,
            _physical_memory[oldest_frame].pid,
            oldest_frame);

    /* Update page table of victim process */
    int victim_pid = _physical_memory[oldest_frame].pid;
    int victim_page = _physical_memory[oldest_frame].page_number;

    if (victim_pid > 0 && victim_pid <= MAXPROC && _proctable[victim_pid - 1] != NULL)
    {
        ScisSosPCB *victim_pcb = _proctable[victim_pid - 1];
        if (victim_page >= 0 && victim_page < MAXPGES)
        {
            victim_pcb->pg_table[victim_page][1] = EMPTY;
        }
    }

    _mem_stats.total_page_replacements++;
    return oldest_frame;
}

/* LRU Page Replacement Algorithm */
int page_replace_lru(int pid, int page_number)
{
    int lru_frame = 0;
    long lru_time = _physical_memory[0].last_reference;

    /* Find the frame with the least recent reference */
    for (int i = 1; i < _num_frames; i++)
    {
        if (_physical_memory[i].last_reference < lru_time)
        {
            lru_time = _physical_memory[i].last_reference;
            lru_frame = i;
        }
    }

    fprintf(stdout, "[LRU] Replacing page %d of PID %d in frame %d (last_ref=%ld)\n",
            _physical_memory[lru_frame].page_number,
            _physical_memory[lru_frame].pid,
            lru_frame,
            lru_time);

    /* Update page table of victim process */
    int victim_pid = _physical_memory[lru_frame].pid;
    int victim_page = _physical_memory[lru_frame].page_number;

    if (victim_pid > 0 && victim_pid <= MAXPROC && _proctable[victim_pid - 1] != NULL)
    {
        ScisSosPCB *victim_pcb = _proctable[victim_pid - 1];
        if (victim_page >= 0 && victim_page < MAXPGES)
        {
            victim_pcb->pg_table[victim_page][1] = EMPTY;
        }
    }

    _mem_stats.total_page_replacements++;
    return lru_frame;
}

/* Clock (Second Chance) Page Replacement Algorithm */
int page_replace_clock(int pid, int page_number)
{
    int victim_frame = EMPTY;
    int iterations = 0;
    int max_iterations = _num_frames * 2; /* Prevent infinite loop */

    while (iterations < max_iterations)
    {
        /* Check if current frame has reference bit cleared */
        if (!(_physical_memory[clock_hand].flags & DIRTY_BIT))
        {
            victim_frame = clock_hand;
            clock_hand = (clock_hand + 1) % _num_frames;
            break;
        }

        /* Clear reference bit and move to next frame */
        _physical_memory[clock_hand].flags &= ~DIRTY_BIT;
        clock_hand = (clock_hand + 1) % _num_frames;
        iterations++;
    }

    /* If no victim found, use current clock position */
    if (victim_frame == EMPTY)
    {
        victim_frame = clock_hand;
        clock_hand = (clock_hand + 1) % _num_frames;
    }

    fprintf(stdout, "[CLOCK] Replacing page %d of PID %d in frame %d\n",
            _physical_memory[victim_frame].page_number,
            _physical_memory[victim_frame].pid,
            victim_frame);

    /* Update page table of victim process */
    int victim_pid = _physical_memory[victim_frame].pid;
    int victim_page = _physical_memory[victim_frame].page_number;

    if (victim_pid > 0 && victim_pid <= MAXPROC && _proctable[victim_pid - 1] != NULL)
    {
        ScisSosPCB *victim_pcb = _proctable[victim_pid - 1];
        if (victim_page >= 0 && victim_page < MAXPGES)
        {
            victim_pcb->pg_table[victim_page][1] = EMPTY;
        }
    }

    _mem_stats.total_page_replacements++;
    return victim_frame;
}

/* Load a page into physical memory */
int memory_load_page(int pid, int page_number)
{
    if (pid < 1 || pid > MAXPROC || _proctable[pid - 1] == NULL)
    {
        fprintf(stderr, "[MEMORY] Error: Invalid PID %d\n", pid);
        return -1;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    /* Validate page number */
    if (page_number < 0 || page_number >= pcb->num_pages)
    {
        fprintf(stderr, "[MEMORY] Error: Invalid page number %d for PID %d (max: %d)\n",
                page_number, pid, pcb->num_pages - 1);
        return -1;
    }

    /* Check if page is already loaded */
    int frame = is_page_loaded(pid, page_number);
    if (frame != EMPTY)
    {
        /* Page hit - update reference */
        memory_update_reference(pid, page_number);
        return frame;
    }

    /* Page fault occurred */
    pcb->page_faults++;
    _mem_stats.total_page_faults++;

    fprintf(stdout, "[PAGE FAULT] PID %d - Page %d (Fault #%d)\n",
            pid, page_number, pcb->page_faults);

    /* Find a free frame */
    frame = find_free_frame();

    /* If no free frame, use page replacement */
    if (frame == EMPTY)
    {
        /* Use LRU by default - can be changed */
        frame = page_replace_lru(pid, page_number);
    }

    /* Load the page into the frame */
    _physical_memory[frame].page_number = page_number;
    _physical_memory[frame].pid = pid;
    _physical_memory[frame].flags = DIRTY_BIT;
    _physical_memory[frame].last_reference = _global_reference_counter++;
    _physical_memory[frame].load_time = (int)_global_reference_counter;

    /* Update process page table */
    if (page_number >= 0 && page_number < MAXPGES)
    {
        pcb->pg_table[page_number][1] = frame;
    }

    _mem_stats.total_page_loads++;

    fprintf(stdout, "[MEMORY] Loaded page %d of PID %d into frame %d\n",
            page_number, pid, frame);

    return frame;
}

/* Handle page fault */
int memory_handle_page_fault(int pid, int page_number)
{
    return memory_load_page(pid, page_number);
}

/* Get frame number for a given logical address */
int memory_get_frame(int pid, int logical_address)
{
    if (!_memory_manager_enabled)
    {
        return 0; /* Memory manager disabled */
    }

    if (pid < 1 || pid > MAXPROC || _proctable[pid - 1] == NULL)
    {
        return -1;
    }

    ScisSosPCB *pcb = _proctable[pid - 1];

    /* Calculate page number from logical address */
    int page_number = logical_address / PAGESIZE;
    int offset = logical_address % PAGESIZE;

    /* Validate page number */
    if (page_number < 0 || page_number >= pcb->num_pages)
    {
        fprintf(stderr, "[MEMORY] Error: Logical address %d maps to invalid page %d for PID %d\n",
                logical_address, page_number, pid);
        return -1;
    }

    /* Check page table first */
    if (page_number >= 0 && page_number < MAXPGES)
    {
        int frame = pcb->pg_table[page_number][1];
        if (frame != EMPTY && frame >= 0 && frame < _num_frames)
        {
            /* Page is loaded - update reference */
            memory_update_reference(pid, page_number);
            return frame;
        }
    }

    /* Page fault - load the page */
    return memory_load_page(pid, page_number);
}

/* Update reference information for a page */
void memory_update_reference(int pid, int page_number)
{
    int frame = is_page_loaded(pid, page_number);
    if (frame != EMPTY && frame >= 0 && frame < _num_frames)
    {
        _physical_memory[frame].flags |= DIRTY_BIT;
        _physical_memory[frame].last_reference = _global_reference_counter++;

        /* Update process reference counter */
        if (pid > 0 && pid <= MAXPROC && _proctable[pid - 1] != NULL)
        {
            _proctable[pid - 1]->reference_counter++;
        }
    }
}

/* Print memory statistics */
void memory_print_stats(FILE *output)
{
    fprintf(output, "\n=== Memory Manager Statistics ===\n");
    fprintf(output, "Page Size: %d bytes\n", PAGESIZE);
    fprintf(output, "Total Page Faults: %d\n", _mem_stats.total_page_faults);
    fprintf(output, "Total Page Replacements: %d\n", _mem_stats.total_page_replacements);
    fprintf(output, "Total Page Loads: %d\n", _mem_stats.total_page_loads);
    fprintf(output, "Number of Frames: %d\n", _num_frames);

    /* Count used frames */
    int used_frames = 0;
    for (int i = 0; i < _num_frames; i++)
    {
        if (_physical_memory[i].pid != EMPTY)
        {
            used_frames++;
        }
    }
    fprintf(output, "Used Frames: %d/%d (%.1f%%)\n",
            used_frames, _num_frames,
            (used_frames * 100.0) / _num_frames);

    /* Calculate page fault rate */
    if (_global_reference_counter > 0)
    {
        fprintf(output, "Page Fault Rate: %.2f%%\n",
                (_mem_stats.total_page_faults * 100.0) / _global_reference_counter);
    }

    fprintf(output, "==================================\n");
}