/*******************************************************************

   mab - memory management functions for HOST dispatcher

   MabPtr memChk (MabPtr arena, int size);
      - check for memory available (any algorithm)
 
    returns address of "First Fit" block or NULL

   MabPtr memAlloc (MabPtr arena, int size);
      - allocate a memory block
 
    returns address of block or NULL if failure

   MabPtr memFree (MabPtr mab);
      - de-allocate a memory block
 
    returns address of block or merged block

   MabPtr memMerge(Mabptr m);
      - merge m with m->next
 
    returns m

   MabPtr memSplit(Mabptr m, int size);
      - split m into two with first mab having size
  
    returns m or NULL if unable to supply size bytes

*******************************************************************/

#include "mab.h"
#include <assert.h>

/*******************************************************
 * MabPtr memChk (MabPtr arena, int size);
 *    - check for memory available 
 *
 * returns address of "First Fit" block or NULL
 *******************************************************/
MabPtr memChk(MabPtr arena, int size)
{

// you need to implement it.
    MabPtr node = arena;
    while (node) {
        //printf("line 47 node->size %d, node->offset %d, node->allocated %d \n", node->size, node->offset, node->allocated);
        
        if ((node->allocated == 0) && (node->size >= size)) {
            return node; // available node with appropriate size
        }
        node = node->next;
    }
    // return null if cannot find
    return NULL;

}
      
/*******************************************************
 * MabPtr memAlloc (MabPtr arena, int size);
 *    - allocate a memory block
 *
 * returns address of block or NULL if failure
 *******************************************************/
MabPtr memAlloc(MabPtr arena, int size)
{
    // check if avaible block exists
    MabPtr m = memChk(arena, size);
    if (m == NULL) {
        return NULL; // fail to allocate
    }
    
    // check if need split block
    if (m->size > size) {
        m = memSplit(m, size);
    }
    
    assert(m != NULL);
    assert(m->size == size);
    
    // set block allocated
    m->allocated = 1;
    
    // return address of allocated block
    return m;
    
}

/*******************************************************
 * MabPtr memFree (MabPtr mab);
 *    - de-allocate a memory block
 *
 * returns address of block or merged block
 *******************************************************/
MabPtr memFree(MabPtr m)
{
    m->allocated = 0; // set block free
    
    // merge blocks with previous continuous free block
    if (m->prev != NULL && m->prev->allocated == 0) {
        m->prev = memMerge(m->prev);
        m = m->prev;
    }
    
    // merge blocks with the follow continuous free block
    while (m->next && m->next->allocated == 0) {
        m = memMerge(m);
    }
    return m;

}
      
/*******************************************************
 * MabPtr memMerge(Mabptr m);
 *    - merge m with m->next
 *
 * returns m
 *******************************************************/
MabPtr memMerge(MabPtr m)
{
    MabPtr n;

    if (m && (n = m->next)) {
        m->next = n->next;
        m->size += n->size;
        
        free (n);
        if (m->next) (m->next)->prev = m;
    }
    return m;
}

/*******************************************************
 * MabPtr memSplit(MabPtr m, int size);
 *    - split m into two with first mab having size
 *
 * returns m or NULL if unable to supply size bytes
 *******************************************************/
MabPtr memSplit(MabPtr m, int size)
{
    MabPtr n;
    
    if (m) {
        if (m->size > size) {
            n = (MabPtr) malloc( sizeof(Mab) );
            if (!n) {
                fprintf(stderr,"memory allocation error\n");
                exit(127);
            }
            n->offset = m->offset + size;
            n->size = m->size - size;
            m->size = size;
            n->allocated = m->allocated;
            n->next = m->next; // add new node n after m.
            m->next = n; // m before n.
            n->prev = m;
            if (n->next) n->next->prev = n;
        }
        if (m->size == size) return m; // return m. the size of m is size required
    }
    return NULL;    
}
