#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include "ScisosMem.h"

/* Declare external variables from the library */
extern int GRANULE;
extern float MT_VAR[3];
extern int WS_RATIO[3];
extern float WS_FRAC[3];
extern float TR_FRAC[3];

/* Signal handler for floating point exceptions */
static jmp_buf fpe_env;
static void fpe_handler(int sig)
{
    (void)sig;
    longjmp(fpe_env, 1);
}

/* Test with different GRANULE values */
void test_with_granule(int granule_value)
{
    printf("\n=== Testing with GRANULE = %d ===\n", granule_value);
    GRANULE = granule_value;

    /* Print all relevant values */
    printf("MT_VAR: [%.2f, %.2f, %.2f]\n", MT_VAR[0], MT_VAR[1], MT_VAR[2]);
    printf("WS_RATIO: [%d, %d, %d]\n", WS_RATIO[0], WS_RATIO[1], WS_RATIO[2]);
    printf("WS_FRAC: [%.2f, %.2f, %.2f]\n", WS_FRAC[0], WS_FRAC[1], WS_FRAC[2]);
    printf("TR_FRAC: [%.2f, %.2f, %.2f]\n", TR_FRAC[0], TR_FRAC[1], TR_FRAC[2]);

    /* Set up signal handler */
    signal(SIGFPE, fpe_handler);

    if (setjmp(fpe_env) == 0)
    {
        /* Try to generate addresses */
        int size = 10; /* Start with small size */
        int mtype = 3; /* MT_GOOD */

        printf("Calling memory_gen_addrefstrings(size=%d, mtype=%d)...\n", size, mtype);
        int *addresses = memory_gen_addrefstrings(size, mtype);

        if (addresses == NULL)
        {
            printf("ERROR: Function returned NULL\n");
        }
        else
        {
            printf("SUCCESS! Generated addresses: ");
            for (int i = 0; i < size && i < 5; i++)
            {
                printf("%d ", addresses[i]);
            }
            printf("...\n");
            free(addresses);
        }
    }
    else
    {
        printf("FLOATING POINT EXCEPTION caught with GRANULE=%d\n", granule_value);
    }
}

int main(void)
{
    printf("=== Debugging ScisSosMem Library ===\n");
    printf("PAGESIZE: %d\n", PAGESIZE);
    printf("NUMFRAMES: %d\n", NUMFRAMES);

    /* Test with different GRANULE values */
    int granule_values[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, PAGESIZE};
    int num_tests = sizeof(granule_values) / sizeof(granule_values[0]);

    for (int i = 0; i < num_tests; i++)
    {
        test_with_granule(granule_values[i]);
    }

    printf("\n=== Testing Complete ===\n");
    return 0;
}