#ifndef MAB_H
#define MAB_H
/*******************************************************************
  mab.h - Memory Allocation definitions and prototypes for HOST dispatcher

  MabPtr memChk (MabPtr arena, int size); - check for memory available
  MabPtr memAlloc (MabPtr arena, int size); - allocate a memory block
  MabPtr memFree (MabPtr mab); - de-allocate a memory block
  MabPtr memMerge(Mabptr m); - merge m with m->next
  MabPtr memSplit(Mabptr m, int size); - split m into two

*******************************************************************/

#include <stdlib.h>
#include <stdio.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* memory management *******************************/

#define MEMORY_SIZE       1024

struct mab {
    int offset;
    int size;
    int allocated;
    struct mab * next;
    struct mab * prev;
};

typedef struct mab Mab;
typedef Mab * MabPtr; 

/* memory management function prototypes ********/

MabPtr memChk(MabPtr, int);
MabPtr memAlloc(MabPtr, int);
MabPtr memFree(MabPtr);
MabPtr memMerge(MabPtr);   
MabPtr memSplit(MabPtr, int);
#endif
