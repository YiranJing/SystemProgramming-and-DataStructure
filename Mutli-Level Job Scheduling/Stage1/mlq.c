/*
    COMP3520 Assignment 3 Stage 1 - Multi-Level Queue Dispatcher

    usage:
        make
        ./mlq <TESTFILE>
        where <TESTFILE> is the name of a job list
*/

/* Include files */
#include "mlq.h"

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

    // Step 1: initialize the Queues (normal job dispatch queue, real time job
    // dispatch queue, level 0 queue, level 1 queue, level 2 queue)
    PcbPtr Normal_job_dispatch_queue = NULL; // store all normal jobs 
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
    
    
    // Step 2: Fill Job dispatch queue from job dispath file
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
        if (fscanf(input_list_stream,"%d, %d, %d",
             &(process->arrivaltime),
             &(process->servicetime),
             &(process->priority)) != 3) {
            free(process);
            continue;
        }
        
        process->remainingcputime = process->servicetime;
        process->status = PCB_INITIALIZED;
        
        // enqueue the job
        // real-time job being loaded in the RT_job_dispatch_queue
        if (process->priority == 0) {
            RT_job_dispatch_queue = enqPcb(RT_job_dispatch_queue, process);
        // normal job being loaded in the
        } else if (process->priority == 1){
            Normal_job_dispatch_queue = enqPcb(Normal_job_dispatch_queue,
                                               process);
        } else {
            printf("Invalid process read!\n\n");
            free(process);
        }
    }
    
//  Step 3: Ask the user to enter an integer value for time_quantum
    puts("Please enter the value for time_quantum (int): ");
    scanf("%d", &time_quantum);
   
//   Step 4: While there is a currently running prcesses or either
//   queue is not empty
    while (current_process || L0_queue || L1_queue || L2_queue
           || Normal_job_dispatch_queue || RT_job_dispatch_queue) {
        
        //printf("timer is %d \n", timer);
       
//      i. Unload any arrived pending process from the job dispatch queue

//      dequeue process from RT Job Dispatch Queue and enqueue on level 0 queue
//      dequeue process from normal Job Dispatch Queue and enqueue on level 1 queue
        
        // move the real-time job to the Level 0 queue
        while (RT_job_dispatch_queue
               && RT_job_dispatch_queue->arrivaltime <= timer) {
            PcbPtr new_enterProcess = deqPcb(&RT_job_dispatch_queue);
            L0_queue = enqPcb(L0_queue, new_enterProcess);
        }
        
        // move normal job to the Level 1 queue
        while (Normal_job_dispatch_queue
               && Normal_job_dispatch_queue->arrivaltime <= timer) {
            PcbPtr new_enterProcess = deqPcb(&Normal_job_dispatch_queue);
            L1_queue = enqPcb(L1_queue, new_enterProcess);
        }
        
        
        
//      ii. If there is a currently running process
//      deals with the current process
        if (current_process) {
            
//          a. Decrement the process's remaining_cpu_time by quantum;
            current_process->remainingcputime -= quantum;
            
//          b. If the process's allocated time has expired:
            if (current_process->remainingcputime <= 0) {
               
//              A. Terminate the process;
                terminatePcb(current_process);
                
//              B. update accmumlated time
                n_process++; // increase the number of process
                accumulated_service_times += current_process->servicetime;
                accumulated_arrival_times += current_process->arrivaltime;
                accumulated_finish_times += timer;
                
//              C. Deallocate the PCB (process control block)'s memory
                free(current_process);
                current_process = NULL;
            }
//          c. the current running process cannot finish within the time_quantum
            // suspend the current job if L1 or L2 Queue is not empty
            else if (L0_queue || L1_queue) {
//               A. Suspend the currently running process
                
                suspendPcb(current_process);
                
//              B. Add this process to the L2 quque
                
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
            
//          d. suspend the job with priority = 1
            else if (current_process->priority == 1) {
                
                suspendPcb(current_process);
                
                current_process->priority = 2; // modify priority = 2
                L2_queue = enqPcb(L2_queue, current_process);
                    current_process = NULL;
            }
        }
        

//      iii. If there is no running process and there is a process ready to run:
        if (!current_process && (L0_queue || L1_queue || L2_queue)) {

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

        
//      iv. Calculate the time_quantum
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
        
//      If either job dispatch queue is not empty,
//      but cuurrently no job arrive and all queues are empty
        
        // if either normal job dispatch queue or RT job dispatch queue is
        // not empty and multi-level queues are all empty
        else if ((Normal_job_dispatch_queue || RT_job_dispatch_queue)
                 && !L0_queue && !L1_queue && !L2_queue){
            quantum = 1; // sleep one second
        }
        else { // end of story
            quantum = 0;
        }
         

//      v. Let the dispatcher sleep for quantum;
        sleep(quantum);

//      vi. Increment the dispatcher's timer;
        timer += quantum;
  
//      vii. Go back to Step 4.
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
    
//  Step 5. Terminate the job dispatcher
    exit(EXIT_SUCCESS);
}
