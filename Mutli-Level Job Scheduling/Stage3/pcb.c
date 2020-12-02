/*******************************************************************

PCB management functions for HOST dispatcher
 
  pcb - process control block functions for HOST dispatcher

   PcbPtr startPcb(PcbPtr process) - start (or restart) a process
    returns:
      PcbPtr of process
      NULL if start (restart) failed

   PcbPtr suspendPcb(PcbPtr process) - suspend a process
    returns:
      PcbPtr of process
      NULL if suspend failed

   PcbPtr terminatePcb(PcbPtr process) - terminate a process
    returns:
      PcbPtr of process
      NULL if terminate failed

   PcbPtr printPcb(PcbPtr process, FILE * iostream)
    - print process attributes on iostream
    returns:
      PcbPtr of process

   void printPcbHdr(FILE *) - print header for printPcb
    returns:
      void
      
   PcbPtr createnullPcb(void) - create inactive Pcb.
    returns:
      PcbPtr of newly initialised Pcb
      NULL if malloc failed

   PcbPtr enqPcb (PcbPtr headofQ, PcbPtr process)
      - queue process (or join queues) at end of queue
      - enqueues at "tail" of queue. 
    returns head of queue

   PcbPtr deqPcb (PcbPtr * headofQ);
      - dequeue process - take Pcb from "head" of queue.
    returns:
      PcbPtr if dequeued,
      NULL if queue was empty
      & sets new head of Q pointer in adrs at 1st arg

   PcbPtr enqPcbHd (PcbPtr headofQ, PcbPtr process)
      - queue process (or join queues) at front of queue
      - enqueues at "head" of queue. 
    returns head of queue


 *******************************************************************/

#include "pcb.h"
#include <assert.h>
/*******************************************************
 * PcbPtr startPcb(PcbPtr process) - start (or restart)
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if start (restart) failed
 ******************************************************/
PcbPtr startPcb (PcbPtr p) 
{ 
    if (p->pid == 0) {                 // not yet started
        switch (p->pid = fork ()) {    //  so start it
            case -1: 
                perror ("startPcb");
                exit(1); 
            case 0:                             // child 
                p->pid = getpid();
                p->status = PCB_RUNNING;
                printPcbHdr(stdout);            // printout in child to
                printPcb(p, stdout);            //  sync with o/p
                fflush(stdout);
                execvp (p->args[0], p->args); 
                perror (p->args[0]);
                exit (2);
        }                                       // parent         

    } else { // already started & suspended so continue
        kill (p->pid, SIGCONT);
    }    
    p->status = PCB_RUNNING;
    return p; 
} 

/*******************************************************
 * PcbPtr suspendPcb(PcbPtr process) - suspend
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if suspend failed
 ******************************************************/
 PcbPtr suspendPcb(PcbPtr p)
 {
     int status;
     
     kill(p->pid, SIGTSTP);
     waitpid(p->pid, &status, WUNTRACED);
     p->status = PCB_SUSPENDED;
     return p;
 }
 
/*******************************************************
 * PcbPtr terminatePcb(PcbPtr process) - terminate
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if terminate failed
 ******************************************************/
PcbPtr terminatePcb(PcbPtr p)
{
    int status;
    
    kill(p->pid, SIGINT);
    waitpid(p->pid, &status, WUNTRACED);
    p->status = PCB_TERMINATED;
    return p;
}  

/*******************************************************
 * PcbPtr printPcb(PcbPtr process, FILE * iostream)
 *  - print process attributes on iostream
 *  returns:
 *    PcbPtr of process
 ******************************************************/
 
PcbPtr printPcb(PcbPtr p, FILE * iostream)
{
    fprintf(iostream, "%7d%7d%7d%7d%7d%7d  ",
        (int) p->pid, p->arrivaltime, p->priority,
        p->remainingcputime,
        p->memoryblock->offset, p->mbytes);
    switch (p->status) {
        case PCB_UNINITIALIZED:
            fprintf(iostream, "UNINITIALIZED");
            break;
        case PCB_INITIALIZED:
            fprintf(iostream, "INITIALIZED");
            break;
        case PCB_READY:
            fprintf(iostream, "READY");
            break;
        case PCB_RUNNING:
            fprintf(iostream, "RUNNING");
            break;
        case PCB_SUSPENDED:
            fprintf(iostream, "SUSPENDED");
            break;
        case PCB_TERMINATED:
            fprintf(iostream, "PCB_TERMINATED");
            break;
        default:
            fprintf(iostream, "UNKNOWN");
    }
    fprintf(iostream,"\n");
    
    return p;     
}
   
/*******************************************************
 * void printPcbHdr(FILE *) - print header for printPcb
 *  returns:
 *    void
 ******************************************************/  
 
void printPcbHdr(FILE * iostream) 
{  
    fprintf(iostream,"    pid arrive  prior    cpu offset Mbytes status\n");

}
       
/*******************************************************
 * PcbPtr createnullPcb() - create inactive Pcb.
 *
 * returns:
 *    PcbPtr of newly initialised Pcb
 *    NULL if malloc failed
 ******************************************************/
 
PcbPtr createnullPcb()
{
    PcbPtr newprocessPtr;
      
    if ((newprocessPtr = (PcbPtr) malloc (sizeof(Pcb)))) {
        newprocessPtr->pid = 0;
        newprocessPtr->args[0] = DEFAULT_PROCESS;
        newprocessPtr->args[1] = NULL;
        newprocessPtr->arrivaltime = 0;
        newprocessPtr->priority = 2;
	newprocessPtr->servicetime = 0;
        newprocessPtr->remainingcputime = 0;
        newprocessPtr->mbytes = 0;
        newprocessPtr->memoryblock = NULL;
        newprocessPtr->status = PCB_UNINITIALIZED;
        newprocessPtr->next = NULL;
        return newprocessPtr;
    }
    perror("allocating memory for new process");
    return NULL;
}


/*******************************************************
 * PcbPtr enqPcb (PcbPtr headofQ, PcbPtr process)
 *    - queue process (or join queues) at end of queue
 * 
 * returns head of queue
 ******************************************************/
 
PcbPtr enqPcb(PcbPtr q, PcbPtr p)
{
    PcbPtr h = q;
    
    p->next = NULL; 
    if (q) {
        while (q->next) q = q->next;
        q->next = p;
        return h;
    }
    return p;
}


/*******************************************************
* PcbPtr findMaxRemainingTime(PcbPtr   headofQ)
*    - find the job with the longest remaining execution time
*
* returns:
     PcbPtr
     NULL if queue was empty
******************************************************/
PcbPtr findMaxRemainingTime(PcbPtr hPtr) {
    
    PcbPtr q = hPtr;
    PcbPtr res = hPtr;
    if (q) {
        int max_remaining_time = q->remainingcputime;
        
        while (q) {
            if (q->remainingcputime > max_remaining_time) {
                max_remaining_time = q->remainingcputime;
                res = q; // update the result
            }
            q = q->next;
        }
        //printf("findMaxRemainingTime function, pid is %d \n", res->pid);
        return res; // return the node with the longest remaining cpu time
    }
    return NULL; // empty queue
}

/*******************************************************
* PcbPtr removeSpecificJob(PcbPtr headofQ, PcbPtr target)
*    - remove the target node from the queue based on pid
*
* returns:
     PcbPtr: the header of queue after remove the target node
     NULL if queue was empty
******************************************************/
PcbPtr removeSpecificJob(PcbPtr hPtr, PcbPtr target) {
    
    PcbPtr q = hPtr;
    int find = 0;
    
    // if the head itself is the node we search
    if (q->pid == target->pid) {
        find = 1;
        hPtr = hPtr->next;
        
        assert(find == 1);
        return hPtr;
    }
    
    while (q->next) {
        
        if (q->next->pid == target->pid) {
            find = 1;
            // remove this node from list
            q->next = q->next->next;
            break;
        }
        q = q->next;
    }
    
    assert(find == 1);
    return hPtr;
}


/*******************************************************
 * PcbPtr deqPcb (PcbPtr * headofQ);
 *    - dequeue process - take Pcb from head of queue.
 *
 * returns:
 *    PcbPtr if dequeued,
 *    NULL if queue was empty
 *    & sets new head of Q pointer in adrs at 1st arg
 *******************************************************/
 
PcbPtr deqPcb(PcbPtr * hPtr)
{
    PcbPtr p;
     
    if (hPtr && (p = * hPtr)) {
        * hPtr = p->next;
        return p;
    }
    return NULL;
}
/*******************************************************
 * PcbPtr enqPcbHd (PcbPtr headofQ, PcbPtr process)
 *    - queue process (or join queues) at frount of queue
 * 
 * returns head of queue
 ******************************************************/
 
PcbPtr enqPcbHd(PcbPtr q, PcbPtr p)
{
    if (q)
        p->next = q;
    else
        p->next = NULL;

    return p;
}
