# Mutli-Level Job Scheduling 

## Advanced Round Robin method in uniprocessor System
- Author: Yiran Jing
- Date: Nov 2020
## Stage 1: Multi-Level Queue dispatcher (Round Robin based)
![](https://paper-attachments.dropbox.com/s_B73DFD051B72E585F082377A505484F70FB98CB3927F2C7969561B4A2D9D00EB_1606952961697_Screen+Shot+2020-12-03+at+10.49.19+am.png)

- [Pseudo code](https://github.com/YiranJing/SystemProgramming-and-DataStructure/blob/master/Mutli-Level%20Job%20Scheduling/Stage1/Report%20Stage%201.pdf)

Usage:

    $ cd Stage1
    $ make
    $ ./mlq <TESTFILE> # ./mlq <TESTFILE>


## Stage 2: Simple Memory Management
- Add a simple **First Fit memory allocation** scheme to the multi-level queue dispatcher
- manage memory allocation based on **memory allocation block** in a **doubly linked list**
![](https://paper-attachments.dropbox.com/s_B73DFD051B72E585F082377A505484F70FB98CB3927F2C7969561B4A2D9D00EB_1606953150483_Screen+Shot+2020-12-03+at+10.52.21+am.png)

    struct mab {
    int offset; //starting location of the block int size; //size of the block
    int allocated;//whether allocated, or free struct mab * next;
    struct mab * prev;
    };
- [Pseudo code](https://github.com/YiranJing/SystemProgramming-and-DataStructure/blob/master/Mutli-Level%20Job%20Scheduling/Stage2/Report%20Stage%202.pdf)

Usage:

    $ cd Stage2
    $ make
    $ ./mlq <TESTFILE> # ./mlq <TESTFILE>

**Potential Problem**
There we assumed that there are two types of jobs, i.e., `real-time jobs`, and `normal jobs`. Real-time jobs are given a higher priority and will be scheduled to run only if the required memory can be allocated. 

One potential problem with that scheme is that high priority real-time jobs may be blocked for a long time before being admitted into the system for execution if the memory space is all occupied by long running low priority jobs. Thus, we consider temporarily swap out (*see stage 3*)

## Stage 3: Swapping Management

One way to alleviate the problem above is to **temporarily swap out some low priority jobs**


1. When a real-time job “arrives”, if there is no sufficient free memory, a “Level-2” or “priority 2” job will be swapped out of main memory and the allocated memory is freed. The freed memory will immediately be allocated to the new real-time job
2. When a Level-2 job is swapped out, it will be queued to the end (or tail) of a `Swapped job queue`
3. When there are several Level-2 jobs, for swapping we select one which has **the longest remaining execution time**.
4. Jobs in the Swapped job queue can be swapped in and scheduled to run again only when all the Arrived job queues and the Three-level ready queue are empty.
5. When being able to be swapped in, jobs in the Swapped job queue are swapped in and scheduled to run one by one in a `FCFS` manner


- [Pseudo code](https://github.com/YiranJing/SystemProgramming-and-DataStructure/blob/master/Mutli-Level%20Job%20Scheduling/Stage3/Report%20Stage%203.pdf)

Usage

    $ cd Stage3
    $ make
    $ ./mlq <TESTFILE> # ./mlq <TESTFILE>

