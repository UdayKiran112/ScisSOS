/****
 *  Constants for Virtual Memory
 ****/
#define PAGESIZE 4096 /* Size of a Page of memory */
#define NUMFRAMES 64  /* No of Frames in Physical Memory */

/** Variation in the locality of reference during memory addressing **/
extern float MT_VAR[3];
extern int WS_RATIO[3];

/**** Constants for Memory Management *********************************/
extern float WS_FRAC[3]; /* Fraction of time in WS */
extern float TR_FRAC[3]; /* Transitions = 1 - WS_FRAC */
extern int GRANULE;

/* Function declarations */
int _memory_gen_WS(int **, int, int, int);
float *_memory_gen_fractions(int, int, int);
int _memory_gen_addr(int *, int);

/* Main function to generate memory address reference strings */
int *memory_gen_addrefstrings(int size, int mtype);