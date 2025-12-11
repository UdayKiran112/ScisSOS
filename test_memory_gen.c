#include <stdio.h>
#include <stdlib.h>
#include "ScisosMem.h"

/* Simple test to verify ScisSosMem library is working */
int main(void)
{
    printf("=== Testing ScisSosMem Library ===\n\n");

    int size = 100;              /* 100 memory references */
    int mem_types[] = {3, 4, 5}; /* MT_GOOD, MT_BAD, MT_UGLY */
    const char *type_names[] = {"GOOD", "BAD", "UGLY"};

    for (int t = 0; t < 3; t++)
    {
        int mtype = mem_types[t];
        printf("Testing memory type: %s (%d)\n", type_names[t], mtype);

        /* Generate address reference string */
        int *addresses = memory_gen_addrefstrings(size, mtype);

        if (addresses == NULL)
        {
            fprintf(stderr, "Error: Failed to generate addresses for type %d\n", mtype);
            continue;
        }

        /* Print first 20 addresses */
        printf("First 20 addresses: ");
        for (int i = 0; i < 20 && i < size; i++)
        {
            printf("%d ", addresses[i]);
        }
        printf("\n");

        /* Calculate page references (assuming PAGESIZE = 4096) */
        int page_faults = 0;
        int loaded_pages[100] = {0}; /* Track loaded pages */
        int num_loaded = 0;

        for (int i = 0; i < size; i++)
        {
            int page = addresses[i] / PAGESIZE;

            /* Check if page is already loaded */
            int found = 0;
            for (int j = 0; j < num_loaded; j++)
            {
                if (loaded_pages[j] == page)
                {
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                page_faults++;
                if (num_loaded < 100)
                {
                    loaded_pages[num_loaded++] = page;
                }
            }
        }

        printf("Total page faults (unlimited frames): %d\n", page_faults);
        printf("Unique pages accessed: %d\n", num_loaded);
        printf("Page fault rate: %.2f%%\n\n", (page_faults * 100.0) / size);

        /* Free the allocated memory */
        free(addresses);
    }

    printf("=== Test Complete ===\n");
    return 0;
}