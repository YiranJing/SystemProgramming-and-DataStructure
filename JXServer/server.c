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
#include <sys/sysinfo.h>
#include "server.h"
#include "helper_fun.h"
#include "data_structure.h"



/*
    Read compression.dictionary and store as bit array
 */
struct compress_dict *read_compression_dir() {
    
    // initialise compress_dict structure
    struct compress_dict *compress_dict = calloc(1,
                                                 sizeof(struct compress_dict));
    compress_dict->tree = tree_init();
    // unsigned long filelen;
    // total number of bytes in compression directory
    unsigned long filelen = 0;
    uint8_t *byte_array = read_compression_byte(&filelen);
    
    // index of bit (current position) in the new "dict_bit_array"
    // after removing length of bytes
    int index = 0;
    int i = 0;  // index of bit in the original compression directory
    int count = 0; // count number of bytes
    
    struct node_tree *root = compress_dict->tree->root;
    while (i < filelen * 8){
        
        uint8_t segment_len = 0; // the length of one segment
        for (int j = 0; j < 8 && i < filelen * 8; j++){
            if (get_bit(byte_array, i++) == 1)
                set_bit(&segment_len, j);
        }
        // get the starting index of current segment
        compress_dict->seg_index[count] = index;
        // reset the root at the begining of checking each segment
        root = compress_dict->tree->root;
        uint8_t direction = 0;
        struct node_tree *node = NULL;
        for (int j = 0; j < segment_len; j++){
            // update bitarray
            if ((direction = get_bit(byte_array, i++)) == 1){
                set_bit(compress_dict->dict_bit_array, index);
            }
            index++;
            // create new node and insert to the binary tree
            if (j == segment_len - 1)
                node = create_node(1, count);
            else
                node = create_node(0, count);
            // insert node to the binary bree based on the direction
            root = insert_node(root, node, direction);
        }
        count++;
    }
    return compress_dict;
}


/*
    Decode and update payload message and payload length
*/
void decode_msg(struct client_data *data) {
    uint8_t* decom_message = NULL;
    struct compress_dict * compress_dict = data->compress_dict;
    
    // get the length of number of paddings in the msg
    uint8_t padding_num = data->payload_msg[data->payload_int - 1];

    // total number of bit codes in msg
    int number_bits = (data->payload_int - 1) * 8 - padding_num;
    
    // the root of binary tree at the begining of checking each segment
    struct node_tree* root = compress_dict->tree->root;
    uint64_t decompress_len = 0; // count the number of segments
    // Decompression: read bits from the compressed payload
    for (int i = 0; i < number_bits; i++) {
        
        uint8_t bit = get_bit(data->payload_msg, i);
        //printf("%u ", bit);
        if (bit == 1) { // go to right
            root = root->right;
        }
        else {
            root = root->left;
        }
        // decode the current node
        if (root->is_leaf == 1) {
            // read segment number
            decompress_len++;
            decom_message = realloc(decom_message,
                                    decompress_len * sizeof(uint8_t));
            // add the decode value
            decom_message[decompress_len - 1] = root->data;
            // reset the root of binary tree before decoding the next segment
            root = compress_dict->tree->root;
        }
    }
    // update payload msg
    data->payload_msg = decom_message;
    data->payload_int = decompress_len;
}


/*
    Compress the given char array "response"
        response:
            char array we want to compress
        payload_length:
            number of bytes in response
        compress_length:
            int pointer to records the number of bytes of the compression_message
 
    Return compression_message:
            char array to store compressed information
 */
unsigned char *encode_msg(struct client_data *data, uint8_t *response,
                          int payload_length, int *compress_length) {
    
    int number_bit = 0;
    int compress_len = 1; // length of compressed data
    unsigned char *compression_message = malloc(1);
    // iteration: encode each byte
    for (int i = 0; i < payload_length; i++) {
        // get one segment from paylaod msg
        uint8_t c = response[i]; // 改成uint8_t (c from 0-255)
        // get the index of the given byte in the compression dictionary
        int start_index = data->compress_dict->seg_index[c];
        int end_index = data->compress_dict->seg_index[c + 1];
        
        // create new msg containing encode information
        // get bit based on index
        for (int j = start_index; j < end_index; j++) {
            
            if (number_bit == compress_len * 8){
                compress_len++;
                compression_message = realloc(compression_message,
                                              compress_len);
            }
            // modify compressed msg based on compressed directory
            if (get_bit(data->compress_dict->dict_bit_array, j) == 1){
                set_bit(compression_message, number_bit++);
            } else{
                clear_bit(compression_message, number_bit++);
            }
        }
    }
    // append 0 padding to the compressed msg
    char padding_num = abs(number_bit - compress_len * 8);
    for (int i = number_bit; i  < compress_len * 8; i++) {
        clear_bit(compression_message, i);
    }
    compression_message = realloc(compression_message, ++compress_len);
    // the last byte is the number of paddings in the msg
    compression_message[compress_len - 1] = padding_num;
    *compress_length = compress_len;
    return compression_message;
}


/*
    Compress and update msg in the data received from client
 */
void encode_data(struct client_data *data){
    
    int compress_length = 1; // length of compressed data
    unsigned char *compression_message = encode_msg(data, data->payload_msg,
                                                    data->payload_int,
                                                    &compress_length);
    data->payload_int = compress_length;
    data->payload_msg = compression_message;
}




/*
    Read IPv4 address and port from the config_file
    Store them in server struct
 */
struct server read_address_and_port(char *file_name) {
    
    // read binary file？“
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
    server.target_dir = malloc(size + 1);
    
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
    // AF_INET: an address family that is used to designate type of addresses
    // that your socket can communicate with (Internet Protocol v4 addresses)
    // SOCK_STREAM: TCP connection
    // the protocol argument is zero, the default protocol for this address
    // family and type shall be used
    // NOTE: socket call does not specify where data will be coming from,
    // nor where it will be going to – it just creates the interface!
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
    (server->address).sin_addr.s_addr = INADDR_ANY;
    /* Setting up the port we want to use
       htons: host-to-network short
       converts a host to TCP/IP network byte order (big-endian)
       this means it works on 16-bit short integer */
    server->address.sin_port = htons(server->port);
    // (optional) Forcefully attaching socket to the given port
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
               option, sizeof(int));

    /* binds the socket to the address and port number specified in addrress
       the server will be delivered connections from clients connecting to
       this address and port*/
    if (bind(server->socket_fd, (struct sockaddr *)&(server->address),
                                 sizeof(struct sockaddr_in)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}


/*
    Creates a new connected socket, and returns a new file descriptor referring to that socket
    After successful accept, , connection is established between client and server, and they are ready to transfer data.
 */
int create_new_connected_socket(struct server *server) {
    
    socklen_t addrlen = sizeof(server->address);
    
    int new_socket = accept(server->socket_fd,
                            (struct sockaddr *)&(server->address), &addrlen);
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
        If invalid type in the message, send error message to client
        and close socket, and return -1
 
    Valid type digit:
        Echo: 0x0 (0000)
        Directory listing: 0x2 (0010)
        File size query: 0x4 (0100)
        Retrueve file: 0x6 (0110)
        Shutdown command: 0x8 (1000)
*/
int check_error_functionality(uint8_t type, int new_socket) {
    if (type != 0x0 && type != 0x2 && type != 0x4 && type != 0x6
        && type != 0x8) {
        /* error functionality */
        error_response(new_socket);
        //close(new_socket);
        return -1;
    }
    return 0;
}



/*
    Helper function for echo
    Send data to client based on given information
 */
void echo_handler(struct client_data *data) {
    //data->digit_type = 0x1;
    uint8_t response_header = (0x1 << 4) | (data->recv_compression << 3)
                                         | (data->reqr_compression << 2);
    
    //uint8_t response_header = header | 0x10;
    send(data->new_socket, &response_header, 1, 0); // send Message header
    unsigned char hexBuffer[SMALL_BUFFER] = {0};
    
    memcpy((char*)hexBuffer, (char*)&data->payload_int, sizeof(int));
     //send payload length in network byte order
    for(int i = 7; i >= 0; i--) {
        send(data->new_socket, &(hexBuffer[i]), 1, 0);
    }
    send(data->new_socket, data->payload_msg, data->payload_int, 0);
}


/*
    Echo functionality
    Send back the same payload as received with type 0x1
 */
void echo(struct client_data *data, uint8_t header) {
    if (data->recv_compression == true) {
        // set the fifth bit to 0, as the msg already be compressed
        data->reqr_compression = false;
        echo_handler(data);
    }
    else { // received data is not compressed
        if (data->reqr_compression == true) {
            // payload data need to be compressed
            
            // the fif-th bit indicate message is compressed
            data->recv_compression = true;
            data->reqr_compression = false;
            encode_data(data);
        }
        echo_handler(data);
    }
}


/*
    Helper function of dir_list
    Send data to client based on given information
 */
void dir_list_handler(struct client_data *data, int payload_length,
                      void *response) {
    //data->digit_type = 0x3;
    uint8_t response_header = (0x3 << 4) | (data->recv_compression << 3)
                                         | (data->reqr_compression << 2);
    
    send(data->new_socket, &response_header, 1, 0); // send Message header
    //send payload length
    //convert int to uint8_t array with size 8, using network byte order
    unsigned char hexBuffer[SMALL_BUFFER] = {0};
    memcpy((char*)hexBuffer, (char*)&payload_length, sizeof(int));
    for(int i = 7; i >= 0; i--) {
        send(data->new_socket, &(hexBuffer[i]), 1, 0);
    }
    // send back file name
    send(data->new_socket, response, payload_length, 0);
}


/*
    Directory listing
    Send a list of all files in the server's target directory with type 0x3
 */
void dir_list(struct client_data *data) {
    
    DIR *dir;
    struct dirent *ent;
    int payload_length = 0;
    uint8_t *response = calloc(DEFAULT_BUFFER_SIZE, sizeof(uint8_t));
    if ((dir = opendir (data->server->target_dir)) == NULL) {
        /* could not open directory */
        perror ("could not open directory");
        return;
    }
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
        // check regular file or not
        if (ent->d_type == DT_REG) {// DT_REG: regular file
            for (int k = 0; k<strlen(ent->d_name); k++) {
                response[payload_length+k] = ent->d_name[k];
            }
            payload_length += (strlen(ent->d_name) + 1);
            // add terminal NULL to the end of each file name
            response[payload_length-1] = 0x00;
        }
    }
    closedir(dir);
    
    response[payload_length-1] = 0x00;
    response = realloc(response, payload_length * sizeof(char));
        
    if (data->reqr_compression == false) {
        // we can send response without compression
        dir_list_handler(data, payload_length, response);
    } else {
        
        data->recv_compression = true;
        data->reqr_compression = false;
        
        int compress_length = 1; // length of compressed data
        unsigned char *compression_message = encode_msg(data, response,
                                                        payload_length,
                                                        &compress_length);
        dir_list_handler(data, compress_length, compression_message);
    }
    free(response);
}



/*
    File size query
    Send back the length of the file of the target filename with type 0x5
 */
void size_query(struct client_data *data) {
    
    // cast uint8_t array to target file name
    char *target_file = (char *)data->payload_msg;
    uint64_t file_size = 0;
    
    //Check if file exist and calculate the size of the given 'target_file'
    //Send an error message if the 'target_file' not exits and return NULL
    char *path = check_regular_file(data, target_file, &file_size);
    free(path);
    if (path == NULL) // target file doesnot exits
        return;
    
    /* check compression request*/
    if (data->reqr_compression == true) {
        data->recv_compression = true;
        data->reqr_compression = false;
        
        file_size = htobe64(file_size); /* swap before compression */
        data->payload_msg = (uint8_t *)&file_size;
        data->payload_int = 8;
        //uint8_t *response = (uint8_t *)&file_size;
        
        encode_data(data);
        
        /* send data to client*/
        uint8_t response_header = (0x5 << 4) | (data->recv_compression << 3)
                                             | (data->reqr_compression << 2);
        send_data(data, response_header);
        return;
    }
    else {
        /* send data to client directly without compression*/
        uint8_t response_header = (0x5 << 4) | (data->recv_compression << 3)
                                             | (data->reqr_compression << 2);
        unsigned char *response = (unsigned char *)&file_size;
        send(data->new_socket,(void*)(&response_header), 1, 0);
        // send payload length
        uint8_t payload_len[BYTE_SIZE]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x08};
        send(data->new_socket, (void*)(&payload_len), 8, 0);
        // represent response as a 8 byte file size (in network byte order)
        //unsigned char *response = (unsigned char *)&file_size; //
        for (int i = 7; i >= 0; i--) { // send data as big endian (0 0 0 ... d)
            send(data->new_socket, (void*)(&response[i]), 1, 0);
        }
    }
}


/*
    Retrieve file
 
    Decompression msg if needed.
    Send compressed data to client
 */
void retrieve_file(struct client_data * data) {
    
    if (data->recv_compression == true)
        decode_msg(data); // decompress message
    
    /* check valid request */
    uint8_t *file_contents = valid_request_retrieve_file(data);
    if (file_contents == NULL)
        return; // invalid request
    
    /* read payload content */
    // read the first 4 bytes
    uint32_t sessionID = parse(data->payload_msg, TYPEDIGIT);
    // convert 8 bytes to 1 uint64_t
    uint64_t start_offset = parse(&data->payload_msg[TYPEDIGIT], BYTE_SIZE);
    // convert 8 bytes to 1 uint64_t
    uint64_t data_length = parse(&data->payload_msg[TYPEDIGIT * 3], BYTE_SIZE);
    // read target file name since 20-th byte
    char * target_file = (char *)&data->payload_msg[TYPEDIGIT * 5];
       
    // check valid multiplex request
    int check = check_sessions(data, data->sessions, sessionID, start_offset,
                               data_length, target_file);
  
    if (check != 0) // invalid request
        return;
    
    /* send data to client */
    send_retrive_file_response(data, data->sessions, sessionID, start_offset,
                               data_length, target_file, file_contents);
    free(file_contents);
}


/*
    Pthread function
    Handle a single connection, in which multiple requests are possible
*/
void *connection_handler(void *arg) {
    
    struct client_data *data = (struct client_data *) arg;
    
    // continuously read message from clients
    while(1) {
        
        uint8_t buffer[MSGINFO]; /*  the first 9 bytes */
        if (check_valid_request(buffer, data) != 0) {
            return NULL; // invalid request or error functionality
        }
        
        /* Readin the payload length */
        read_payload_length(data, buffer);
        // convert convert the payload uint8_t[] to int
        data->payload_int = parse(data->payload_length, BYTE_SIZE);
        
        /* Shutdown command */
        if (data->digit_type == 0x8) {
            close(data->new_socket); // shutdown send and receive operatiion
            free_data(data);
            return NULL;
        }
        /* directory listing, no paypoad in this case */
        if (data->digit_type == 0x2)
            dir_list(data);
        
        /* Read the following message */
        data->payload_msg = realloc(data->payload_msg, data->payload_int);
        
        if (recv(data->new_socket, data->payload_msg,
                 data->payload_int, 0) == 0) {
            free_data(data);  // client send shutdown message to server
            return NULL;
        }
        /* echo functionality */
        if (data->digit_type == 0x0)
            echo(data, buffer[0]); // send data to client
        /* File size query */
        else if (data->digit_type == 0x4)
            size_query(data); // send data to client
        /* Retrieve file */
        else if (data->digit_type == 0x6)
            retrieve_file(data); // send data to client
    }
}


int main (int argc, char **argv) {

    // Usage: ./server <configuration file>
    // <configuration file> is a path to a binary configuration file
    assert(argc == 2);
    // read IPv4 address and port from the config_file
    struct server server = read_address_and_port(argv[1]);
    int option = 1;
    // Create a TCP socket, and bind to the given address and port
    create_bind_socket(&server, &option);
    
    /* Set socket listening for incoming connections */
    // listen: waits for the client to a the server to make a connection
    listen(server.socket_fd, SOMAXCONN); // backlog up to SOMAXCONN
   
    // create binary tree and bit array for 256 segments
    struct compress_dict *compress_dict = read_compression_dir();
    //initialise session struct to handle multiplex
    struct session *sessions = session_init();
    server.n_threads = 0;
    
    // accecpt new connections
    while(1) {
        server.n_threads++;
        server.pthreads = realloc(server.pthreads,
                                  server.n_threads * sizeof(pthread_t));
        
        // creates a new connected socket
        struct client_data *data = client_data_init(&server,
                                                     compress_dict, sessions);
        
        if (pthread_create(&server.pthreads[server.n_threads-1], NULL,
                           connection_handler, data) != 0) {
            perror("unable to create thread");
            return -1;
        }
        
    }
    
    // wait for all threads finish before destroy
    for (int i = 0; i < server.n_threads; i++) {
        pthread_join(server.pthreads[i], NULL);
    }
    
    // destory and free all memory associated with server
    destory_all(compress_dict, sessions, &server);
    close(server.socket_fd);
    exit(0);
}


