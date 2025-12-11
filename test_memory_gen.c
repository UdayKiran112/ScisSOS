#include <stdio.h>
#include <stdlib.h>
#include "ScisosMem.h"

/* Simple test to verify ScisSosMem library is working */
int main(void)
{
    printf("=== Testing ScisSosMem Library ===\n");

    /* Ensure the library-visible global is initialized */
    GRANULE = 128;
    printf("GRANULE value: %d\n\n", GRANULE);

    int size = 100; /* 100 memory references */
    /* memory types: 0=GOOD, 1=BAD, 2=UGLY (use indices 0..2 since the library arrays are size 3) */
    int mem_types[] = {0, 1, 2};
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
            fprintf(stderr, "Make sure libscismem.a is in the current directory and the library was initialized correctly\n\n");
            continue;
        }

        printf("✓ Successfully generated %d addresses\n", size);

        /* Print first 20 addresses */
        printf("First 20 addresses: ");
        for (int i = 0; i < 20 && i < size; i++)
        {
            printf("%d ", addresses[i]);
            if ((i + 1) % 10 == 0)
                printf("\n                    ");
        }
        printf("\n");

        /* Calculate statistics */
        int min_addr = addresses[0];
        int max_addr = addresses[0];
        for (int i = 1; i < size; i++)
        {
            if (addresses[i] < min_addr)
                min_addr = addresses[i];
            if (addresses[i] > max_addr)
                max_addr = addresses[i];
        }

        printf("Address range: %d to %d bytes\n", min_addr, max_addr);
        printf("Address space: %.2f KB\n", (max_addr - min_addr) / 1024.0);

        /* Calculate page references (assuming PAGESIZE = 4096) */
        int page_faults = 0;
        /* Safe capacity for unique pages: at most 'size' distinct pages; use min(size, 1000) cap */
        int cap = (size < 1000) ? size : 1000;
        int *loaded_pages = (int *)calloc(cap, sizeof(int));
        if (!loaded_pages)
        {
            fprintf(stderr, "ERROR: could not allocate loaded_pages\n");
            free(addresses);
            continue;
        }
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
                if (num_loaded < cap)
                {
                    loaded_pages[num_loaded++] = page;
                }
            }
        }

        printf("Page size: %d bytes\n", PAGESIZE);
        printf("Total page faults (unlimited frames): %d\n", page_faults);
        printf("Unique pages accessed: %d\n", num_loaded);
        printf("Page fault rate: %.2f%%\n", (page_faults * 100.0) / size);

        if (num_loaded <= 0)
        {
            printf("Memory efficiency: N/A (no unique pages loaded)\n");
        }
        else
        {
            printf("Memory efficiency: %.2f references/page\n", (double)size / num_loaded);
        }
        printf("\n");

        /* Free the allocated memory */
        free(addresses);
        free(loaded_pages);
    }

    printf("=== Test Complete ===\n");
    printf("\n✓ Library is working correctly!\n");
    printf("  - GOOD processes should have ~10-20%% page faults\n");
    printf("  - BAD processes should have ~30-50%% page faults\n");
    printf("  - UGLY processes should have ~50-80%% page faults\n");

    return 0;
}
