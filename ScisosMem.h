/****
 *  Constants for Virtual Memory
 ****/
#define PAGESIZE    4096                  /* Size of a Page of memory */
#define NUMFRAMES   64                    /* No of Frames in Physical Memory */

/** Variation in the locality of reference during memory addressing **/
float MT_VAR[3] = {8.0, 4.0, 2.0};
int WS_RATIO[3] = {16, 10, 8};

/**** Constants for Memory Management *********************************/
float WS_FRAC[3] = {0.9, 0.7, 0.5};       /* Fraction of time in WS */
float TR_FRAC[3] = {0.1, 0.3, 0.5};       /* Transitions = 1 - WS_FRAC */
int GRANULE;

int _memory_gen_WS(int **, int, int, int);
float *_memory_gen_fractions(int, int, int);
int _memory_gen_addr(int *, int);