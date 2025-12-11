#include "ScisSos.h"

/*Rotating pointer for FIFO replacement  */
static int fifo_ptr = 0;

// Initialize memory management system
void memory_initialise(void)
{
    fprintf(stdout, "=== Initialising Memory Management System ===\n");

    // Initialize frame table
    for (int i = 0; i < NUMFRAMES; i++)
    {
        frame_table[i].page_number = EMPTY;
        frame_table[i].pid = EMPTY;
        frame_table[i].dirty = 0;
        frame_table[i].use = 0;
        frame_table[i].reference_cnt = 0;
    }

    fifo_ptr = 0;
    fprintf(stdout, "Memory management system initialised with %d frames\n", NUMFRAMES);
}

// Page fault handler
int memory_page_fault_handler(int pid, int page)
{
    // Validate inputs
    if (pid < 1 || pid > MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d in page fault handler\n", pid);
        return -1;
    }

    if (page < 0 || page >= MAXPGES)
    {
        fprintf(stderr, "Error: Invalid page number %d for PID %d\n", page, pid);
        return -1;
    }

    fprintf(stdout, "[PAGE FAULT] Process PID %d requesting page %d\n", pid, page);

    // First, check if there's a free frame available
    int free_frame = -1;
    for (int i = 0; i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid == EMPTY)
        {
            free_frame = i;
            break;
        }
    }

    int allocated_frame;

    if (free_frame != -1)
    {
        // Free frame available - allocate it
        allocated_frame = free_frame;
        fprintf(stdout, "[ALLOCATION] Free frame %d allocated to PID %d, page %d\n",
                allocated_frame, pid, page);
    }
    else
    {
        // No free frames - need page replacement using FIFO
        fprintf(stdout, "[REPLACEMENT] No free frames available, using FIFO replacement\n");
        allocated_frame = memory_fifo();

        if (allocated_frame == -1)
        {
            fprintf(stderr, "Error: Page replacement failed\n");
            return -1;
        }

        // Get the victim page information
        int victim_pid = frame_table[allocated_frame].pid;
        int victim_page = frame_table[allocated_frame].page_number;

        fprintf(stdout, "[VICTIM] Frame %d: evicting PID %d, page %d\n",
                allocated_frame, victim_pid, victim_page);

        // Update the victim process's page table
        if (victim_pid >= 1 && victim_pid <= MAXPROC)
        {
            ScisSosPCB *victim_pcb = _proctable[victim_pid - 1];
            if (victim_pcb != NULL && victim_page < MAXPGES)
            {
                victim_pcb->pg_table[victim_page][1] = EMPTY;
                victim_pcb->page_loaded[victim_page] = 0;
                victim_pcb->num_mem_pages--;
                fprintf(stdout, "[UPDATE] PID %d page table updated: page %d unloaded\n",
                        victim_pid, victim_page);
            }
        }
    }

    // Load the new page into the allocated frame
    frame_table[allocated_frame].page_number = page;
    frame_table[allocated_frame].pid = pid;
    frame_table[allocated_frame].dirty = 0;
    frame_table[allocated_frame].use = 1;
    frame_table[allocated_frame].reference_cnt = 1;

    fprintf(stdout, "[LOADED] PID %d, page %d loaded into frame %d\n",
            pid, page, allocated_frame);

    return allocated_frame;
}

// FIFO page replacement algorithm
int memory_fifo(void)
{
    // Start from current FIFO pointer and find next occupied frame
    int start_ptr = fifo_ptr;
    int selected_frame = -1;

    // Find the next occupied frame using circular rotation
    for (int i = 0; i < NUMFRAMES; i++)
    {
        int current_frame = (start_ptr + i) % NUMFRAMES;

        if (frame_table[current_frame].pid != EMPTY)
        {
            selected_frame = current_frame;
            // Move pointer to next position for next replacement
            fifo_ptr = (current_frame + 1) % NUMFRAMES;
            break;
        }
    }

    if (selected_frame == -1)
    {
        fprintf(stderr, "Error: No frames available for replacement\n");
        return -1;
    }

    fprintf(stdout, "[FIFO] Selected frame %d for replacement\n", selected_frame);
    return selected_frame;
}

// Get a free frame or allocate one for a process
int memory_get_page(int pid, int page)
{
    // Validate inputs
    if (pid < 0 || pid >= MAXPROC)
    {
        fprintf(stderr, "Error: Invalid PID %d\n", pid + 1);
        return -1;
    }

    if (page < 0 || page >= MAXPGES)
    {
        fprintf(stderr, "Error: Invalid page number %d\n", page);
        return -1;
    }

    // Check if page is already loaded
    ScisSosPCB *pcb = _proctable[pid];
    if (pcb != NULL && pcb->page_loaded[page])
    {
        int frame = pcb->pg_table[page][1];
        fprintf(stdout, "[CACHE HIT] PID %d page %d already in frame %d\n",
                pid + 1, page, frame);
        return frame;
    }

    // Look for a free frame
    for (int i = 0; i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid == EMPTY)
        {
            // Found free frame
            frame_table[i].page_number = page;
            frame_table[i].pid = pid + 1;
            frame_table[i].dirty = 0;
            frame_table[i].use = 1;
            frame_table[i].reference_cnt = 1;

            fprintf(stdout, "[ALLOCATED] Frame %d allocated to PID %d, page %d\n",
                    i, pid + 1, page);
            return i;
        }
    }

    // No free frames available
    fprintf(stdout, "[WARNING] No free frames available for PID %d, page %d\n",
            pid + 1, page);
    return -1;
}

// Print memory statistics
void print_memory_stats(void)
{
    fprintf(stdout, "\n=== Memory Statistics ===\n");
    fprintf(stdout, "Total Frames: %d\n", NUMFRAMES);

    int used_frames = 0;
    int free_frames = 0;

    for (int i = 0; i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid != EMPTY)
        {
            used_frames++;
        }
        else
        {
            free_frames++;
        }
    }

    fprintf(stdout, "Used Frames: %d\n", used_frames);
    fprintf(stdout, "Free Frames: %d\n", free_frames);
    fprintf(stdout, "Total Page Faults: %d\n", page_faults);

    // Print frame table details (first 10 frames for brevity)
    fprintf(stdout, "\nFrame Table (First 10 frames):\n");
    fprintf(stdout, "Frame | PID  | Page | Dirty | Use | Ref Count\n");
    fprintf(stdout, "------+------+------+-------+-----+----------\n");
    for (int i = 0; i < 10 && i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid != EMPTY)
        {
            fprintf(stdout, " %3d  | %4d | %4d |   %d   |  %d  |    %d\n",
                    i, frame_table[i].pid, frame_table[i].page_number,
                    frame_table[i].dirty, frame_table[i].use,
                    frame_table[i].reference_cnt);
        }
        else
        {
            fprintf(stdout, " %3d  | FREE |  --  |  --   | --  |    --\n", i);
        }
    }
    fprintf(stdout, "========================\n\n");
}

// LRU page replacement algorithm (optional implementation)
int memory_lru(void)
{
    int lru_frame = -1;
    int min_ref_count = __INT_MAX__;

    // Find frame with minimum reference count
    for (int i = 0; i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid != EMPTY)
        {
            if (frame_table[i].reference_cnt < min_ref_count)
            {
                min_ref_count = frame_table[i].reference_cnt;
                lru_frame = i;
            }
        }
    }

    if (lru_frame == -1)
    {
        fprintf(stderr, "Error: No frames available for LRU replacement\n");
        return -1;
    }

    fprintf(stdout, "[LRU] Selected frame %d (ref_count=%d) for replacement\n",
            lru_frame, min_ref_count);
    return lru_frame;
}