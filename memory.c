#include "ScisSos.h"

void memory_initialise(void)
{
    
}
int memory_page_fault_handler(int pid, int page);    /* Handle page faults */
int memory_get_page(int pid, int address);           /* Get page number for address */
int memory_fifo(void);                               /* FIFO page replacement */
int memory_lru(void);                                /* LRU page replacement */
void print_memory_stats(void);                       /* Print memory stats */
int *memory_gen_addrefstrings(int size, int m_type); /* Generate address reference strings */
