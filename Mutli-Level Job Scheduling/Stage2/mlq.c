/*
    COMP3520 Assignment 3 Stage 2 - Simple Memory Management

    usage:
        make
        ./mlq <TESTFILE>
        where <TESTFILE> is the name of a job list
 
 */

/* Include files */
#include "mlq.h"
#include <assert.h>

// global variable
int time_quantum = 0;
int quantum = 0;

/*Function to find minimum of x and y */
int min(int x, int y) {
    return y ^ (-(x < y) & (x ^ y));
}


int main (int argc, char *argv[])
{
    /* Main function variable declarations */

    FILE * input_list_stream = NULL;

    // Step 1: initialize the Queues
    
    PcbPtr Job_dispatch_queue = NULL; // store all the jobs from the job list
    
    PcbPtr Normal_job_dispatch_queue = NULL; // store all normal jobs when they arrive
    PcbPtr RT_job_dispatch_queue = NULL; // real time job dispatcher
    
    PcbPtr L0_queue = NULL; // level 0 queue
    PcbPtr L1_queue = NULL; // level 1 queue
    PcbPtr L2_queue = NULL; // level 2 queue
    
    PcbPtr process = NULL;
    PcbPtr current_process = NULL;
    int timer = 0;// simulator time start
    
    /* initialise variables for turnaround time and waiting time calculation */
    int n_process = 0; // count the total number of jobs
    int accumulated_arrival_times = 0;
    int accumulated_service_times = 0;
    int accumulated_finish_times = 0;

    
//  Step 2: initialise the simulated memory block
    // the simulated memory is size of 1GB
    MabPtr Memory_blocks;
    Memory_blocks = (MabPtr) malloc(MEMORY_SIZE);
    Memory_blocks->size = MEMORY_SIZE;
    Memory_blocks->next = NULL;
    Memory_blocks->prev = NULL;
    Memory_blocks->offset = 0;
    Memory_blocks->allocated = 0;
    
    
    
    // Step 3: Fill Job dispatch queue from job dispath file
    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <TESTFILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!(input_list_stream = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while (!feof(input_list_stream)) {  // put processes into job dispatch_queue
        process = createnullPcb();
        if (fscanf(input_list_stream,"%d, %d, %d, %d",
             &(process->arrivaltime),
             &(process->servicetime),
             &(process->priority), &(process->mbytes)) != 4) {
            free(process);
            continue;
        }
        
        process->remainingcputime = process->servicetime;
        process->status = PCB_INITIALIZED;
        
        // enqueue the job to job dispatcher queue
        Job_dispatch_queue = enqPcb(Job_dispatch_queue, process);
    }
    
//  Step 4: Ask the user to enter an intger value for time_quantum
    puts("Please enter the value for time_quantum (int): ");
    scanf("%d", &time_quantum);
   
//   Step 5: While there is a currently running prcesses or either
//   queue is not empty
    while (current_process || L0_queue || L1_queue || L2_queue
           || Normal_job_dispatch_queue || RT_job_dispatch_queue
           || Job_dispatch_queue) {
        
//      i. Set check_job_addmission = 0
        // require consider admission of arrived jobs in
        // the arrived job queues immediately or not
        int check_job_addmission = 0;
        
        //printf("timer is %d \n", timer);
        
//      ii. Unload any arrived pending process from the job dispatch queue

//      dequeue process from RT Job Dispatch Queue and enqueue on level 0 queue
//      dequeue process from normal Job Dispatch Queue and enqueue on level 1 queue
        while (Job_dispatch_queue && Job_dispatch_queue->arrivaltime <= timer) {
            PcbPtr new_enterProcess = deqPcb(&Job_dispatch_queue);
            // check priority of the queue
            // move the real-time job to the Level 0 queue
            if (new_enterProcess->priority == 0) {
                RT_job_dispatch_queue = enqPcb(RT_job_dispatch_queue,
                                               new_enterProcess);
            } else {
               // move normaal job to the Level 1 queue
                Normal_job_dispatch_queue = enqPcb(Normal_job_dispatch_queue,
                                                   new_enterProcess);
            }
        }
       
//      iii. If there is current running process and the process's allocated time has expired:
        if (current_process && (current_process->remainingcputime - quantum) <= 0) {
                    
//          a. Decrement the process's remaining_cpu_time by quantum;
            current_process->remainingcputime -= quantum;
                       
//          b. Terminate the process;
            terminatePcb(current_process);
            
//          c. update accmumlated time
            n_process++; // increase the number of process
            accumulated_service_times += current_process->servicetime;
            accumulated_arrival_times += current_process->arrivaltime;
            accumulated_finish_times += timer;
                    
//          d. Deallocate the memory block's memory
            memFree(current_process->memoryblock); // free memory block
            
//          e. Deallocate the PCB (process control block)'s memory
            free(current_process); // free process
            current_process = NULL;
                        
//          f. consider admission of arrived jobs in the arrived job queues immediately
            check_job_addmission = 1;
        }
        
        // iv. job admission check and enqueue available jobs to level 0 or level 1 queue
        
        // a. IF arrived RT job queue is empty, check normal  job admission:
        //    only when arrived RT job queue is empty
        //    jobs in the arrived normal job queue can be considered for admission
        if (RT_job_dispatch_queue == NULL) { // empty queue
            
            while(Normal_job_dispatch_queue){
                // check if memory can be allocated using first fit allocation scheme
                int size = Normal_job_dispatch_queue->mbytes;
                MabPtr m = memAlloc(Memory_blocks, size);
                if (m != NULL) { // find appropriate location
                    // dispatch job
                    PcbPtr new_enterProcess = deqPcb(&Normal_job_dispatch_queue);
                    new_enterProcess->memoryblock = m;
                    L1_queue = enqPcb(L1_queue, new_enterProcess);
                    assert(L1_queue);
                }
                else {
                    break; // cannot find appropriate location for the next job
                }
            }
        }
        
        // b. Else if arrived RT job queue is not empty, check RT job admission:
        while (RT_job_dispatch_queue) {
            
            // check if memory can be allocated using first fit allocation scheme
            int size = RT_job_dispatch_queue->mbytes;

            MabPtr m = memAlloc(Memory_blocks, size);
            if (m != NULL) { // find appropriate location
                // dispatch job
                PcbPtr new_enterProcess = deqPcb(&RT_job_dispatch_queue);
                new_enterProcess->memoryblock = m;
                L0_queue = enqPcb(L0_queue, new_enterProcess);
            }
            else {
                break; // cannot find appropriate location for the next job
            }
        }
        
        

//      v. If there is a currently running process;
        if (current_process) {
            
//          a. Decrement the process's remaining_cpu_time by quantum;
            current_process->remainingcputime -= quantum;
            
//          b. the current running process cannot finish within the time_quantum
            // suspend the current job if L1 and L2 Queue is not empty
            if (L0_queue || L1_queue) {
//              A. Suspend the currently running process
                suspendPcb(current_process);
                
//              B. Add this process to L2 quque
                // case 1: append job to the end of L2 queue, if priority is 1
                if (current_process->priority == 1) {
                    current_process->priority = 2; // modify priority = 2
                    L2_queue = enqPcb(L2_queue, current_process);
                }
                // case 2: append job to the front of L2 queue, if priority is 2
                else { //priority = 2
                    L2_queue = enqPcbHd(L2_queue, current_process);
                }
                current_process = NULL;
            }
            
            // suspend the job with priority = 1 
            else if (current_process->priority == 1) {
                
                suspendPcb(current_process);
                current_process->priority = 2; // modify priority = 2
                L2_queue = enqPcb(L2_queue, current_process);
                current_process = NULL;
            }
        }
        

//      vi. If we donot need to check job addmission immediately,
        // and there is no running process and there is a process ready to run:
        if ((check_job_addmission == 0) && (!current_process)
            && (L0_queue || L1_queue || L2_queue)) {
            
//          a. Dequeue the process at the head of the queue
//             and set it to current_process
            // we first check level 0 queue
            if (L0_queue) {
                
                current_process = deqPcb(&L0_queue);
            } else if (L1_queue) { // then check l1 queue
                current_process = deqPcb(&L1_queue);
            } else { // pop from l2 queue
                current_process = deqPcb(&L2_queue);
            }
            
//          b. If the process job is a suspended process
            if (current_process->status == PCB_SUSPENDED) {
//              send SIGCONT signal to resume it
                printPcbHdr(stdout);
                // print the status of the given process
                printPcb(current_process, stdout);
                startPcb(current_process);

            }
            else {
//          c. else start it and set its status as running
               startPcb(current_process);
            }
        }
        
//      vii. Calculate the time_quantum
        
        // A. if there is current running process
        if (current_process) {
            
            // for RT job in level 0 queue, quantum is its remaining_cpu_time
            if (current_process->priority == 0) {
                quantum = current_process->remainingcputime;
            }
            // for the normal job,  quantum is the minimum value
            // of time_quamtum and remaining_cpu_time
            else if (current_process->priority == 1) {
                quantum = min(time_quantum, current_process->remainingcputime);
            }
            else { // priority = 2
               // for the jobs in level 2, set the quantum = 1,
               // and keep check every second
                quantum = 1;
            }
        }
        
//      B. If Job_dispatch_queue is not empty,
//      but all other queues are empty
        
        // if the job dispatch queue is not empty
        // but both normal job dispatch queue and RT job dispatch queue is empty
        // and multi-level queues are all empty
        else if (Job_dispatch_queue && !Normal_job_dispatch_queue
                 && !RT_job_dispatch_queue
                 && !L0_queue && !L1_queue && !L2_queue){
            quantum = 1; // sleep one second
        }
        else { // end of story
            quantum = 0;
        }
         
//      viii. Let the dispatcher sleep for quantum;
        sleep(quantum);

//      ix. Increment the dispatcher's timer;
        timer += quantum;
  
//      x. Go back to Step 5.
    }
    
    // Calculate the average turnaround time
    // turnaround time = finish time - Arrival time
    float avg_turnaorund_time = (accumulated_finish_times -
                                 accumulated_arrival_times)/(float)n_process;
    
    
    // Calculate the average waiting time
    // waiting time = turnaround time - service time
    float avg_waiting_time = (accumulated_finish_times - accumulated_arrival_times
                            - accumulated_service_times)/(float)n_process;
    
    printf("The average turnaround time is: %f\n", avg_turnaorund_time);
    printf("The average waiting time is: %f\n", avg_waiting_time);
    
//  Step 6: Free the memory of memory block and then terminate the job dispatcher
    free(Memory_blocks);
    exit(EXIT_SUCCESS);
}
