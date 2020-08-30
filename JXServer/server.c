#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <endian.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include "server.h"
#include "thread_pool.h"
#include "helper_fun.h"

#define COMPRESSION_DIR ("compression.dict")


// clang -g -Wall -Werror -std=gnu11 server.c -o server -lpthread
// ./server config_file

/*
    Networked server:
    This server will listen for and accept connections from clients
*/

/* function for bit array
 
 ref:
 http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
 */


void set_bit(uint8_t* array, int index){
    int i = index/8; // index within bit array
    int pos = index%8;

    uint8_t flag = 1;

    flag = flag << (8-pos-1); // (BYTE_SIZE - 1 - x % BYTE_SIZE)
    array[i] = array[i] | flag;
}


void clear_bit(uint8_t *array, int index) {
    int i = k/32;
    int pos = k%32;

    unsigned int flag = 1;  // flag = 0000.....00001

    flag = flag << pos;     // flag = 0000...010...000   (shifted k positions)
    flag = ~flag;           // flag = 1111...101..111

    array[i] = array[i] & flag;
}

//https://www.sanfoundry.com/c-program-implement-bit-array/
unit8_t get_bit(uint8_t *array, int index) {
    
    unsigned int mask = 1 & (array[index / 8] >> (index % 8));
    
    uint8_t result = mask & 1;
    
    return result;
}


/*
    Read and store binary file to byte array
    https://stackoverflow.com/questions/28269995/c-read-file-byte-by-byte-using-fread
*/
uint8_t * read_compression_dir(long * filelen) {

    FILE *fileptr;
    uint8_t *buffer;
    int i;
    // read binary file
    fileptr = fopen(COMPRESSION_DIR, "rb");
    fseek(fileptr, 0, SEEK_END);
    *filelen = ftell(fileptr);
    rewind(fileptr); // sets the file position to the beginning of the file
    buffer = malloc((*filelen)*sizeof(uint8_t));
    for(i = 0; i < *filelen; i++) {
       fread(buffer+i, 1, 1, fileptr);  // read one byte each time
    }
    fclose(fileptr);
    return buffer;
}


















/*
    Read IPv4 address and port from the config_file
    Store them in server struct
 */
struct server read_address_and_port(char *file_name) {
    
    // read binary file
    FILE *config_fd = fopen(file_name, "rb");  // r for read, b for binary
    if (config_fd == NULL) {
        //printf("Fail to read file [%s]\n", file_name);
        exit(EXIT_FAILURE);
    }

    // First 4 bytes: address in network byte order
    struct in_addr inaddr;
    fread(&(inaddr.s_addr), sizeof(inaddr.s_addr), 1, config_fd);
    char *address = inet_ntoa(inaddr); // convert from 32 bit-address to dotted quad notation

    // Next 2 bytes: port in network byte order
    uint16_t port;
    fread(&port, sizeof(port), 1, config_fd); // read to our buffer
    // Convert port number to network byte order
    port = htons((uint16_t) port);

    struct server server = { .inaddr = inaddr, .port = port, .addr = address};
    
    // Remainder: directory as non-NULL terminated ASCII
    fseek(config_fd, 0, SEEK_END);
    size_t size = ftell(config_fd) - 6;
    fseek(config_fd, 6, SEEK_SET);
    server.target_dir = malloc(size+1);
    //server.target_dir = malloc(size);
    
    fread(server.target_dir, size, 1, config_fd);
    server.target_dir[size] = '\0';
    fclose(config_fd);
    //printf("the name of target directory is: %s", server.target_dir);
    return server;
}


/*
    Create a TCP (reliable, connection oriented) socket
    and bind to the given address and port
 */
void create_bind_socket(struct server *server, int *option) {
    
    /* create a socket */
    //AF_INET: an address family that is used to designate the type of addresses
    //that your socket can communicate with (Internet Protocol v4 addresses)
    //SOCK_STREAM: TCP connection
    //the protocol argument is zero, the default protocol for this address family and type shall be used.
    // NOTE: socket call does not specify where data will be coming from, nor where it will be going to
    // – it just creates the interface!
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("socket");
        return;
    }

    /* Set up address */
    // setting up an address, since different protocols that can be used
    // we need specify what one (family)
    (server->address).sin_family = AF_INET; // this is an IPv4 socket
    // binding the address to any IP on our system and the last field
    // s_addr is variable that holds the information about the address we agree to accept
    // INADDR_ANY: binds the socket to all available interfaces
    // For a server, we typically want to bind to all interfaces - not just "localhost"
    // To accept connctions from all IPs
    (server->address).sin_addr.s_addr = INADDR_ANY;
    // Setting up the port we want to use
    // htons: host-to-network short
    // converts a host to TCP/IP network byte order (big-endian)
    // this means it works on 16-bit short integer
    server->address.sin_port = htons(server->port);
    // helpful part for testing
    // (optional) Forcefully attaching socket to the given port
    // manipulating options for the socket referred by the file descriptor socket_fd
    // helps in reuse of address and port. Prevents error such as: “address already in use”.
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
               option, sizeof(int));

    /* binds the socket to the address and port number specified in addrress */
    /* the server will be delivered connections from clients connecting to this address and port*/
    if (bind(server->socket_fd, (struct sockaddr *)&(server->address),
                                 sizeof(struct sockaddr_in))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}


/*
    Creates a new connected socket, and returns a new file descriptor referring to that socket
    After successful accept, , connection is established between client and server, and they are ready to transfer data.
 */
int create_new_connected_socket(struct server * server) {
    
    socklen_t addrlen = sizeof(server->address);
    
    int new_socket = accept(server->socket_fd, (struct sockaddr *)&(server->address), &addrlen);
    if (new_socket < 0) {
        perror ("accept");
        exit(EXIT_FAILURE);
    }
    return new_socket;
}


/*
    Helper function for the error functionality
 */
void error_response(int new_socket) {
    uint8_t response[MSGINFO] = {0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00};
    send(new_socket, response, MSGINFO, 0);
}



/*
    Error functionality
        If invalid type in the message, send error message to client and close socket, and return -1
 
 Valid type digit:
     Echo: 0x0 (0000): return 0
     Directory listing: 0x2 (0010): 2
     File size query: 0x4 (0100):  4
     Retrueve file: 0x6 (0110):  6
     Shutdown command: 0x8 (1000):  8
*/
int check_error_functionality(char type, int new_socket) {
    if (type != '0' && type != '2' && type != '4' && type != '6' && type != '8') {
        /* error functionality */
        error_response(new_socket);
        //close(new_socket);
        return -1;
    }
    return 0;
}


/*
    Free data and close the connection
 */
void free_data(struct client_data *data) {
    free(data->payload_msg);
    close(data->new_socket);
    free(data);
}


/*
    echo functionality
 */
void echo(struct client_data * data, uint8_t header) {
    
    if (data->reqr_compression == '0') { // check requires compression bit
        // server response compression is optional, so send original mes straignt back to them
        
        //printf("Donot have to compression payload, can send it straignt back to them\n");
        uint8_t response_header = 0x10;
        
        if (data->recv_compression == '0') { // the received msg is compressed
            // set the fifth bit to 1, as the msg send back is compressed
            response_header = header | 0x10;
        }
        
        send(data->new_socket, &response_header, 1, 0); // send Message header
        send(data->new_socket, data->payload_length, 8, 0); // send payload length
        send(data->new_socket, data->payload_msg, data->payload_int, 0); // send byte payload
    }
    else {
        // all server response must be compressed
        printf("must compression payload, before send back\n");
        
        // todo:
        // in the new header, fifth bit nneds to indicate msg has been compressed
        // sisth bit indicates it no longer requires compression
        
        
        // compress payload
    }
}


/*
    Directory listing
 */
void dir_list(struct client_data * data) {
    
    DIR *dir;
    struct dirent *ent;
    int payload_length = 0;
    char * response = calloc(100, sizeof(char)); //有问题, 不可以这样，
    
    if (data->reqr_compression == '0') {
        
        if ((dir = opendir (data->server->target_dir)) == NULL) {
            /* could not open directory */
            perror ("could not open directory");
            return;
        }
        
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
              // check regular file or not
              // value 8 is a regular file
              if (ent->d_type == 8) {
                  
                  for (int k = 0; k<strlen(ent->d_name); k++) {
                      response[payload_length+k] = ent->d_name[k];
                  }
                  payload_length += (strlen(ent->d_name) + 1);
                  response[payload_length-1] = 0x00; // add terminal NULL to the end of each filename
                  //printf("file name %s, length %ld \n", ent->d_name, strlen(ent->d_name));
                  //printf("payload_length %d \n", payload_length);
              }
         }
        closedir(dir);
        response[payload_length-1] = 0x00;
        response = realloc(response, payload_length * sizeof(char));
        
        if (data->recv_compression == '0') {
            
            uint8_t response_header = 0x30;
            send(data->new_socket, &response_header, 1, 0); // send Message header
            //send payload length
            //convert int to uint8_t array with size 8, using network byte order
            unsigned char hexBuffer[100]={0};
            memcpy((char*)hexBuffer,(char*)&payload_length,sizeof(int));
            for(int i=7;i>=0;i--){
                send(data->new_socket, &(hexBuffer[i]),1,0);
            }
            // send back file name
            send(data->new_socket, (void*)(response), payload_length, 0);
        }
        else {
            // compress file name (response )
        }
    }
    free(response);
}


/*
    File size query
 */
void size_query(struct client_data * data) {
    
    // cast uint8_t array to target file name
    char * target_file = (char *)data->payload_msg;
    uint64_t file_size = 0;
  
    //Check if file exist and calculate the size of the given 'target_file'
    //Send an error message if the 'target_file' not exits and return NULL
    //Return the relative path of  if the target file exist, also calculate file size, saved in 'file_size'
    //int found = 0; // euqal to 1 if the given file exist
    char* path = check_regular_file(data, target_file, &file_size);
    free(path);
    if (path == NULL) { // target file doesnot exits
        return;
    }
    
    /* send data to client*/
    // send Message header
    uint8_t size_header= {0x50}; // header 在不需要compression的时候
    send(data->new_socket,(void*)(&size_header), 1, 0);
    // send payload length
    uint8_t payload_len[MSGINFO-1]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08};
    send(data->new_socket, (void*)(&payload_len), 8, 0);
    // represent response as a 8 byte file size (in network byte order)
    unsigned char *response = (unsigned char *)&file_size; //
    for (int i=7; i >= 0; i--) { // send data as big endian
        //printf("%c \n", response[i]); // 0 0 0 ... d
        send(data->new_socket, (void*)(&response[i]), 1, 0);
    }
     
}


/*
    Retrieve file
 //https://edstem.org/courses/3996/discussion/243605
 //https://edstem.org/courses/3996/discussion/244971 （最后一个test case）
 //https://edstem.org/courses/3996/discussion/245351
 */
void retrieve_file(struct client_data * data) {
    
    /* read payload content */
    uint8_t sessionID[TYPEDIGIT] = {0};
    char * target_file = (char *)&data->payload_msg[TYPEDIGIT*5]; // read target file name since 20-th byte
    memcpy((char*)sessionID,(char*)data->payload_msg,TYPEDIGIT); // read the first 4 bytes
    uint64_t startOffset = parse(&data->payload_msg[TYPEDIGIT], MSGINFO-1); // convert 8 bytes to 1 uint64_t
    uint64_t data_length = parse(&data->payload_msg[TYPEDIGIT*3], MSGINFO-1); // convert 8 bytes to 1 uint64_t
    
    /* check valid request */
    uint64_t file_size = 0;
    //Check if file exist and calculate the size of the given 'target_file'
    char* path = check_regular_file(data, target_file, &file_size);
    if (path == NULL) { // target file doesnot exits
        free(path);
        return;
    }
    // check valid range
    uint64_t request_range = startOffset + data_length;
    if (request_range > file_size) {
        error_response(data->new_socket);
        free(path);
        return;
    }
    
    /* valid request */
    FILE *fd = fopen(path, "r");  // read target file
    if (fd == NULL) {
        printf("Fail to read file [%s]\n", path);
        exit(EXIT_FAILURE);
    }
    //set read pointer
    fseek(fd, startOffset, SEEK_SET);
    uint8_t * file_contents = malloc(data_length * sizeof(uint8_t)); //read all data at once
    fread(file_contents, data_length, 1, fd);
    
    /* send data to client */
    // send Message header
    uint8_t size_header= {0x70}; // header 在不需要compression的时候 这个和前面不一样 如果合并function要注意
    send(data->new_socket, (void*)(&size_header), 1, 0);
    
    // send payload length
    // represent response as a 8 byte file size (in network byte order)
    // payload length = data_length + 20 bytes (sessionID + starting offset + portion lenth)
    uint64_t payload_length = data_length + 20;
    unsigned char *response = (unsigned char *)&payload_length; //
    for (int i=7; i >= 0; i--) { // send length as big endian
        send(data->new_socket, (void*)(&response[i]), 1, 0);
    }
    
    // send payload content
    send(data->new_socket, sessionID, 4, 0); // send sessionID (4 bytes)
    send(data->new_socket, (void*)(&data->payload_msg[TYPEDIGIT]), 8, 0); // send starting offset
    send(data->new_socket, (void*)(&data->payload_msg[TYPEDIGIT*3]), 8, 0); // send portion lenth //这个需要改, 因为或许需要分开send
    send(data->new_socket, (void*)file_contents, data_length, 0); // send file content
    
    free(file_contents);
    fclose(fd);
    free(path);
}


/*
    Pthread function
    Handle a single connection, in which multiple requests are possible
*/
void *connection_handler(void* arg) {
    
    struct client_data * data = (struct client_data *) arg;
    // continuously read message from the client
    while(1) {
        
        //uint8_t header_buffer[1]; /*  the first 1 bytes */
        uint8_t buffer[MSGINFO]; /*  the first 9 bytes */
    
        if (recv(data->new_socket, buffer , MSGINFO, 0) <= 0) {
          // client send shutdown message to server
          free_data(data);
          return NULL;
        }
        
        read_message_header(buffer[0], data); //read degit type, and compression
        
        /* Error functionality: invalid digit type */
        if (check_error_functionality(data->digit_type, data->new_socket) <0) {
            free_data(data);
            return NULL;
        }
        
        
        /* Readin the payload length */
        read_payload_length(data, buffer);
        // convert convert the payload uint8_t[] to int
        data->payload_int = parse(data->payload_length, MSGINFO-1);
        /* Shutdown command */
        // guaranteed a Shutdown command will only be sent after your server completes processing all other requests
        // After all processing has been completed and resources freed, your server should terminate with exit code 0
        if (data->digit_type == '8') {
            close(data->new_socket); // Shutdown both send and receive operations.
            free_data(data);
            return NULL;
        }
        
        /* directory listing, no paypoad in this case */
        if (data->digit_type == '2') {
            dir_list(data);
        }
        
        /* Read the following message */
        data->payload_msg = realloc(data->payload_msg,
                                    data->payload_int * sizeof(uint8_t));
        
        if (recv(data->new_socket, data->payload_msg,
                 data->payload_int, 0) == 0) {
            // client send shutdown message to server
            free_data(data);
            return NULL;
        }
        
        /* echo functionality */
        if (data->digit_type == '0') {
            echo(data, buffer[0]); // send data to client
        }
    
        /* File size query */
        else if (data->digit_type == '4') {
            size_query(data); // send data to client
        }
        
        /* Retrieve file */
        else if (data->digit_type == '6') {
            retrieve_file(data); // send data to client
        }
        
    }
}


int main (int argc, char ** argv) {

    // Usage: ./server <configuration file>
    // <configuration file> is a path to a binary configuration file
    assert(argc == 2);
    
    // read IPv4 address and port from the config_file
    // store them in server struct
    struct server server = read_address_and_port(argv[1]);
    
    int option = 1;
    // Create a TCP (reliable, connection oriented) socket
    // and bind to the given address and port
    create_bind_socket(&server, &option);
    
    /* Set socket listening for incoming connections */
    // listen: waits for the client to a the server to make a connection
    // If a connection request arrives when the queue is full, the client may receive an error
    // The backlog(second arg of listen) argument defines the maximum length to which the queue of
    // pending connections for sockfd may grow.
    listen(server.socket_fd, THREAD_POOL_SIZE); // listen for connections

    /* initialise thread pool */
    struct queue *queue = queue_init();
    thread_pool_new(&server, queue);
    
    while(1) { // accecpt new connections
    
        struct client_data *data = calloc(1, sizeof(struct client_data));
        // creates a new connected socket, and returns a new file descriptor referring to that socket
        data->new_socket = create_new_connected_socket(&server);
        data->server = &server;
        
        pthread_mutex_lock(&queue->lock); // to avoid race condition
        enqueue(queue, data); // available thread can find new connection in the queue
        // sleep if no new connection come
        pthread_cond_signal(&queue->condition_var); // wake up one waiting thread
        pthread_mutex_unlock(&queue->lock);
    }
    
    // wait for all threads finish before destroy
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(server.thread_pool[i], NULL);
    }

    // destroy
    pthread_mutex_destroy(&(queue->lock));
    queue_destory(queue);
    free(server.target_dir);
    close(server.socket_fd);
    exit(0);
}
