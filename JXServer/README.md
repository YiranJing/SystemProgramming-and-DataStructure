# XJServer
- COMP2017 assignment 3
- Date: June 2020
- Author: Yiran Jing

A networked server (“JXServer”) that sends files to clients in response to requests. My server is responsible for creating a "listening" TCP socket. In this project, server use standard TCP with IPv4. Both client and server endpoints are defined by an IP address (32 bit unsigned integer) as well as a TCP port number (16 bit unsigned integer)

## Run
    ./server <configuration file>

The argument `<configuration file>` will be a path to a binary configuration file. The data of
this file will be arranged in the following layout from the start:

- 4 bytes - the IPv4 address your server will listen on, in network byte order
- 2 bytes - representing the TCP port number your server will listen on, in network byte order
- All remaining bytes until the end of the file - ASCII representing the relative or absolute path to the directory (“the target directory”), from which your server will offer files to clients.
## Content
1. [Message layout](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:h2=Message-layout)
2. [Message Functionality](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:h2=Message-layout)
3. [Data deduplication](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:uid=790198515502176518142135&h2=Data-deduplication)
4. [Concurrent request through thread pool](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:uid=273701364310380715848740&h2=Concurrent-requests-using-thre)
5. [**Huffman Coding and Compression**](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:uid=982575259929877457368713&h2=Huffman-Coding-and-Compression)
6. [Lossless compression](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:uid=521688136864916273861424&h2=Lossless-Compression)
7. [Decompression](https://www.dropbox.com/scl/fi/odmpptsd12rc5kdurjndr/Untitled-6.paper?dl=0&rlkey=0bz2wegpxdeda2tjh7wwnhd7t#:uid=896693758375522789853461&h2=Decompression)


----------
## Message layout

Upon a successful connection, clients will send a number of requests. When a cleint is finished making requests on a given connection, they will `shutdown` connection.
Each message will contain the following fields in the below format:

- **1 byte - Message header**:
    - First 4 bits - “Type digit”
    - 5th bit - “Compression bit”
    - 6th bit - “Requires compression bit”
    - 7th and 8th bits - padding bits, which should all be 0.
- **8 bytes - Payload length**
- **Variable byte payload** - a sequence of bytes, with length equal to the length field described previously.

**Payload length**
Although TCP guarantee that data is delivered and in order, it treats data as a continuous stream rather than discrete messages. Thus in `connection_handler` hold for each threads, the message header (first 9 bytes) will be read. Based on the message length given by header, we will further read the message content.
Moreover, the payload length of the sent data is in big-endian. To get the correct payload legnth, convert to little-endian first.

## Message Functionality

- **Error functionality**
when receive unknown type message, send error message back and close connection
- **Echo functionality**
- **Directory listing**
- **File size query**
- **Shutdown command**
- **Retrieve file**
The payload will consist of the following structure:
    - 4 bytes - `session ID` for this request
    - 8 bytes - the starting offset of data
    - 8 bytes - the length of data
    - Variable bytes - target filename

```c
    /* payload in retrieve file functionality */
    struct request {
        uint32_t sessionID; /* the first 4 bytes in payload msg */
        char * target_file; /* file name */
        uint64_t start_offset; /* start offset of data*/
        uint64_t data_length; /* data range */
    };
```

**Session ID**

- **One client** might open several concurrent request on different connections with the **same** sessionID. In this case, only one connection can get the response. 
- **One client** can make concurrent request on **different** files using **different** sessionID.
-  It is invalid to receive a request for a different file with the same sessionID as a currently ongoing transfer.


## Concurrent requests using thread pool

This server supports multiple connecting clients simultaneously as well as multiple simultaneous connections from the same client for increased transfer speeds.

To handle multiple (concurrent) requests, I create a new pthread for each new connection, assign the same function (connection_handler()) to each thread, and keep to increase the number of threads within the while loop. By doing so, my program can respond to multiple connections without blocking on others, and the number of threads is equal to the number of connections.

However, the current method might lead to poor performance when program receives a huge number of connections. since it will have expensive overhead costs due to creating many threads, and the threads that finish early have to wait for other threads (load imbalance). To handle these problems, the thread pool applied.


## Data deduplication

As mentioned above, if one client send the same requests with the same sessionID concurrently, only one connection can get the response. To handle this, we store all currently active sessions in a dynamic array.

    /*Dynamic array to records the sessions of one clinet */
    struct session {
        struct request *requests; /* array to record concurrent sessions */
        uint64_t capacity; /* capacity of sessions */
        uint64_t size; /* current length of sessions */
        pthread_mutex_t lock; /* handle multiplex */
    };
1. When the new request comes, we firstly check if the same request active already. If yes, send empty message back. Otherwise, append the new sessions to array. 
2. After send back the message to client, remove sessionID from the list. 



## [Huffman Coding and Compression](https://github.com/YiranJing/SystemProgramming-and-DataStructure/tree/master/Huffman%20Coding%20and%20Compression)

Huffman encoding is a way to encode texts based on the char’s frequency. The higher-frequency char with the short code words. The value of leaf nodes is from `0x00` to `0xFF` (256 symbols). 

Steps of encoding calculation:

1. calculate the frequency of all symbols in the input file.
2. Create a node for each possible input symbols, with value equal to the frequency.  
3. Create a `mini_heap`, in which all nodes sorted by frequency.
4. Build huffman Tree:

       -  pop two nodes with smallest frequency from heap
       -  create a new node and insert to the binary tree 
       -  recurtively build huff tree until only one node left

5. Save bitcode and the start index of each node (in `compress_dict` as bit array) for decoding. Also save the root of Huffman tree for compression purpose.
    #define DICT_SIZE (256)
    struct compress_dict {
        struct binary_tree *tree; // root of Huffman tree
        uint8_t dict_bit_array[DICT_SIZE * 4]; /* bit array for compression infor */
        int seg_index[DICT_SIZE + 1]; /* start index of 256 segments and padding */
    };
    
    struct node_tree {
      uint8_t data;
      int is_leaf;
      struct node_tree *left;
      struct node_tree *right;
    };
    
    struct binary_tree {
        struct node_tree *root;
    };
## Lossless Compression

see `encode_msg` function in [server.c](https://github.com/YiranJing/SystemProgramming-and-DataStructure/blob/master/JXServer/server.c)


## Decompression 

see `decode_msg` function in [server.c](https://github.com/YiranJing/SystemProgramming-and-DataStructure/blob/master/JXServer/server.c)

