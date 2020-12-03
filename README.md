# SystemProgramming-and-DataStructure
👩🏻‍💻 Data Structre and Concurrent Programming (2020.03 - 2020.07)


## Concurrency Project
### [**JxServer**](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/JXServer)
   - Create a networked server (“JXServer”) that sends files to clients in response to requests.
   - Server supports multiple connecting clients simultaneously as well as multiple simultaneous connections from the same client for increased transfer speeds.
   - [**Huffman Coding and Compression 🌳**](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Huffman%20Coding%20and%20Compression)
      - A program that reads from a specified file, and uses Huffman coding to compress that file. 
      - Use Bitarray and Huffman Tree(binary tree)
   
### [Multi-Level Job Scheduling](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Mutli-Level%20Job%20Scheduling)
   - Advanced Round Robin method in uniprocessor System
   - Stsge 1: Multi-Level Queue Dispatcher
   - Stsge 2: Simple Memory Management
   - Stsge 3: Swapping Strategy

### [Group Lab Exercise](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Lab%20Exercise)
   - Manage concurrent students(threads for each) to do the lab exericse. 
   - Constraints: one teacher (coordinater), and only one lab room to fit one group each time
   - Use mutex and conditional vriable to handle synchronization issue.


***


## Data Structure
1. 🔐 [Thread Safe **Hash Map** (*separate chaining*)](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Thread%20Safe%20HashMap)
   - Half-fine grained locking
   - Buckets: list of dynamic array
   - Collision Handling: Separate chaining. (each cell of hash table point to a linked list)
   - Load factor of HashMap is 0.75f
   
2. [Thread Safe Unrolled Linked List](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Unrolled%20linked%20list)
   - Coarse-grained locking linked list

3. [Office](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Office)
   - The office is structured as a hierarchy where the boss is at the top and everyone else is below.
   - Using a tree data structure to support many different queries.
   

## System Programming 
1. [MPSC (multiple producer, single consumer) channel](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Channels)
2. [Process communication with single central process](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Comms)
3. [Game ND-Minesweeper](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/ND-Minesweeper)
4. [Simple Version Control](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Simple%20Version%20Control)
   - Design and implement the storage method, as well as some functions for Simple Version Control (SVC)
   - (Very) simplified system derived from the Git version control system.


   
