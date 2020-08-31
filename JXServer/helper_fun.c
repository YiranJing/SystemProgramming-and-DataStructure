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
#include "helper_fun.h"
#include "data_structure.h"



/*
    Convert message header from unit8_t to bits
    Store the first 4 bits to digit_type
    Store the fifth bit to recv_compression
    Store the sixth bit to reqr_compresssion
*/
void read_message_header(uint8_t header, struct client_data *data) {
    
    data->digit_type = header >> 4; // conver int to a char
    data->recv_compression = (header >> 3) & 1;
    data->reqr_compression = (header >> 2) & 1;
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
* I declare that the majority of the following function has been copied from
*tutorialspoint website with only minor changes and it is not my own work.
*
* https://www.tutorialspoint.com/learn_c_by_examples/binary_to_hexadecimal_program_in_c.htm
*/
int binary2decimal(char *n, int size) {
    int i, decimal, mul = 0;
    // Check the given digit_type is valid or not (4 bits array)
    for(decimal = 0,i = size-1; i >= 0; --i, ++mul)
      decimal += (n[i] - 48) * (1 << mul);

    return decimal;
}


/*
    Convert input uint8_t array to one uint64_t
 */
uint64_t parse(uint8_t *v, uint8_t size) {
    uint64_t val = v[7];
    int i = 6;
    for (int j = 1; j < size; j++){
        val = val | v[j] << (8 * i);
        i--;
    }
    return val;
}


/*
    Read 8 byte payload length i
 */
void read_payload_length(struct client_data *data, uint8_t *buffer) {
    for (int i = 0; i < 8; i++) {
        data->payload_length[i] = buffer[1 + i];
    }
}


/*
    Helper function of read_compression_dir()
 
    Read and store binary file to byte array (buffer)
    Return buffer
*/
uint8_t *read_compression_byte(unsigned long *filelen) {

    FILE *fileptr;
    uint8_t *buffer;
    fileptr = fopen(COMPRESSION_DIR, "rb"); // read binary file
    fseek(fileptr, 0, SEEK_END);
    *filelen = ftell(fileptr);
    rewind(fileptr); // sets the file position to the beginning of the file
    buffer = malloc((*filelen) * sizeof(uint8_t));
    fread(buffer, 1, *filelen, fileptr); // read and store all bytes to buffer
    fclose(fileptr);
    return buffer;
}


/*
    Helper function of file size query (size_query()),
    and retrieve file (retrieve_file())
 
    Check if file exist and calculate the size of the given 'target_file'
    Send an error message if the 'target_file' not exits, return NULL
    If the target file exist, also calculate file size, saved in 'file_size',
    return the relative path of the target file
 */
char *check_regular_file(struct client_data *data, char *target_file,
                          uint64_t *file_size) {
   
    char *files_in_dir[DEFAULT_BUFFER_SIZE];
    DIR *dir;
    int idx = 0;
    int found = 0; // euqal to 1 if the given file exist
    struct dirent *ent;
    char *path = NULL;
    if ((dir = opendir(data->server->target_dir)) == NULL) {
        // fail to open directory
        perror("could not open directory");
        return NULL;
    }
    while ((ent = readdir(dir)) != NULL) {
        if(ent->d_type == DT_REG) { // DT_REG: regular file
            files_in_dir[idx] = ent->d_name;
            idx+=1;
        }
    }
    // check the requried file exists in the target directory
    for (int i = 0; i < idx; i++) {
        if(strcmp(files_in_dir[i], target_file) == 0) {
            found = 1; // target file exits in directory
            break;
        }
    }
    // if the required file name doesnot exist, return an error response
    if(found == 0) {
        error_response(data->new_socket);
        return NULL;
    }
    
        // the response contains the length of the file with target file name
        path = realloc(path, strlen(data->server->target_dir) + 3 +
                       strlen(target_file));
        strcpy(path, data->server->target_dir);
        strcat(path, "/");
        strcat(path, target_file);
        // get file size
        struct stat stats;
        if(stat(path, &stats) != 0) {
            perror("stats: ");
        }
        // For regular files, the file size in bytes
        *file_size = (uint64_t)stats.st_size;
    return path;
}


/*
    Helper function
    Send payload data to client
 */
void send_data(struct client_data * data, uint8_t response_header) {
    send(data->new_socket, &response_header, 1, 0); // send Message header
    
    //send payload length
    //convert int to uint8_t array with size 8, using network byte order
    unsigned char hexBuffer[SMALL_BUFFER] = {0};
    memcpy((char*)hexBuffer, (char*)&(data->payload_int), sizeof(int));
    
    for(int i = 7; i >= 0; i--) {
        send(data->new_socket, &(hexBuffer[i]), 1, 0);
    }
    
    // send back file name
    send(data->new_socket, data->payload_msg, data->payload_int, 0);
}


/*
    Helper function of retrieve_file
 
    Merge the first 20 bytes from payload_msg,
    and then append retrieve file content in file_contents
    Return "merge_response" with length as payload_length
 */
uint8_t *merge_infor(struct client_data *data, uint8_t * file_contents,
                      uint64_t payload_length) {
    uint8_t *merge_response = malloc(payload_length * sizeof(uint8_t));
    
    // copy the first 20 bytes from payload_msg,
    // including sessionID, starting offset and length
    for (int i = 0; i < payload_length; i++) {
        if (i < 20) {
            merge_response[i] = data->payload_msg[i];
        }
        else {
            // copy file_content to the merge result
            merge_response[i] = file_contents[i - 20];
        }
    }
    return merge_response;
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
    Helper function for retrieve_file
    send empty payload, when find the same concurrent request
 */
void send_empty_payload(struct client_data *data) {
    unsigned char header = {0x70}; // header without compression
    write(data->new_socket, &header, 1);
    uint64_t pay_length = 0;
    unsigned char *result = (unsigned char *)&pay_length;
    for(int i = 7;  i >= 0; i--) {
        write(data->new_socket, &result[i], 1);
    }
}



/*
    Check if we have the same concurrent request with
    the same sessionID and the same file range
    
    Send the error response if the same sessionID require different file
    or different file range
 
    Return 1 if we find the same concurrent requests
    Return -1 if invalid request
    Return 0 if new request not exists on archive
 */
int check_sessions(struct client_data *data, struct session *sessions,
                   uint32_t sessionID, uint64_t start_offset,
                   uint64_t data_length, char *target_file) {

    pthread_mutex_lock(&(sessions->lock));
    
    for (int i = 0; i < sessions->size; i++) {
        struct request *req = &sessions->requests[i];
                
        if (req->sessionID == sessionID) {
            // check valid request
            if (strcmp(req->target_file, target_file) != 0 ||
                req->start_offset != start_offset ||
                req->data_length != data_length ) {
                // invalid request if the sessionID same,
                // but target file name not
                pthread_mutex_unlock(&(sessions->lock));
                // invalid request if the sessionID are same,
                // but target file name not
                error_response(data->new_socket); // send error response
                return -1;
            }
            else { // find the same concurrent request
                pthread_mutex_unlock(&(sessions->lock));
                // send empty payload, as find the same concurrent request
                send_empty_payload(data);
                return 1;
            }
        }
    }
    // cannt find
    // create new session
    if (sessions->size == sessions->capacity) {
        sessions->capacity = sessions->capacity * 2;
        sessions->requests = realloc(sessions->requests,
                                sessions->capacity * sizeof(struct request));
    }
    sessions->size++;
    sessions->requests[sessions->size -1].sessionID = sessionID;
    sessions->requests[sessions->size - 1].target_file = target_file;
    sessions->requests[sessions->size - 1].start_offset = start_offset;
    sessions->requests[sessions->size - 1].data_length = data_length;
               
    pthread_mutex_unlock(&(sessions->lock));
    return 0;
}


/*
    Remove spcific session after job finish
 */
void remove_session(struct session *sessions, uint32_t sessionID,
                    uint64_t start_offset, uint64_t data_length,
                    char *target_file) {
    
    pthread_mutex_lock(&(sessions->lock));
    for (int i = 0; i<sessions->size; i++) {
        struct request *req = &sessions->requests[i];
        if (req->sessionID == sessionID
                              && strcmp(req->target_file, target_file) == 0
                              && req->start_offset == start_offset
                              && req->data_length == data_length) {
            
            sessions->requests[sessions->size -1].sessionID = sessionID;
            sessions->requests[sessions->size - 1].target_file = target_file;
            sessions->requests[sessions->size - 1].start_offset = start_offset;
            sessions->requests[sessions->size - 1].data_length = data_length;
            
            break;
        }
    }
    sessions->size--;
    pthread_mutex_unlock(&(sessions->lock));
}


/*
   Helper function for retrieve_file
   send uncompressed msg to client
*/
void send_uncompression_msg(struct client_data *data, uint64_t data_length,
                            uint8_t *file_contents) {
    // send message header
    uint8_t size_header= {0x70}; // header without compression
    send(data->new_socket, (void*)(&size_header), 1, 0);
    
    // send payload length
    // represent response as a 8 byte file size (in network byte order)
    // payload length: data_length + 20 bytes (ID+ starOffset + portionLenth)
    uint64_t payload_length = data_length + 20;
    unsigned char *response = (unsigned char *)&payload_length;
    for (int i = 7; i >= 0; i--) { // send length as big endian
        send(data->new_socket, (void *)(&response[i]), 1, 0);
    }
    
    uint8_t sessionID[TYPEDIGIT] = {0};
    memcpy((char*)sessionID, (char*)data->payload_msg, TYPEDIGIT);
    // send payload content
    send(data->new_socket, sessionID, 4, 0); // send sessionID (4 bytes)
    // send starting offset
    send(data->new_socket, (void*)(&data->payload_msg[TYPEDIGIT]), 8, 0);
    // send portion lenth
    send(data->new_socket, (void*)(&data->payload_msg[TYPEDIGIT * 3]), 8, 0);
    // send file content
    send(data->new_socket, (void*)file_contents, data_length, 0);
}


/*
   Helper function for retrieve_file
   Send compressed msg to client
*/
void send_compression_msg(struct client_data *data, uint64_t data_length,
                          uint32_t sessionID, uint64_t start_offset,
                          char *target_file, uint8_t *file_contents) {
    data->recv_compression = true;
    data->reqr_compression = false;
    uint8_t response_header = (0x7 << 4) | (data->recv_compression << 3)
                                | (data->reqr_compression << 2);
    uint64_t payload_length = data_length + 20;
    
    // merge the first 20 bytes from payload_msg
    // and then append retrieve file content in file_contents
    uint8_t *merge_response = merge_infor(data, file_contents, payload_length);
    data->payload_int = payload_length;
    data->payload_msg = merge_response;
    encode_data(data); // encode data
    send_data(data, response_header); // send data
    
    // remove sessionID after job finish
    remove_session(data->sessions, sessionID, start_offset, data_length,
                   target_file);
    free(merge_response);
}


/*
    Initialise session struct to handle multiplex
 */
struct session *session_init() {
    
    struct session *sessions = calloc(1, sizeof(struct session));
    sessions->capacity = INIT_CAPACITY;
    sessions->requests = calloc(sessions->capacity, sizeof(struct request));
    pthread_mutex_init(&(sessions->lock), NULL);
    
    return sessions;
}


/*
    Creates a new connected socket
    and initialise a client data struct for the new connection
 */
struct client_data *client_data_init(struct server *server,
                                     struct compress_dict *compress_dict,
                                     struct session *sessions) {
    
    struct client_data *data = calloc(1, sizeof(struct client_data));
    
    // creates a new connected socket,
    // and returns a new file descriptor referring to that socket
    data->new_socket = create_new_connected_socket(server);
    data->server = server;
    data->compress_dict = compress_dict;
    data->sessions = sessions;
    
    return data;
}


/*
    Helper function used to free memory associated with dictionary struct
 */
void destroy_dictionary(struct compress_dict *compress_dict) {
    tree_destroy(compress_dict->tree->root);
    free(compress_dict->tree);
    free(compress_dict);
}


/*
    Destroy and free all memory associated with server
 */
void destory_all(struct compress_dict *compress_dict,
                 struct session *sessions, struct server *server) {
    
    pthread_mutex_destroy(&(sessions->lock));
    free(sessions->requests);
    free(server->pthreads);
    free(sessions);
    destroy_dictionary(compress_dict); //free memory associated with compression
    free(server->target_dir);
}

/*
    Helper function for connection_handler
    
    Check valid request and error functionality
    Return 0 if the request is valid
 */
int check_valid_request(uint8_t *buffer, struct client_data *data) {
    
    if (recv(data->new_socket, buffer , MSGINFO, 0) <= 0) {
        // client send shutdown message to server
        free_data(data);
        return -1;
    }
    
    read_message_header(buffer[0], data); //read degit type, and compression
    
    /* Error functionality: invalid digit type */
    if (check_error_functionality(data->digit_type, data->new_socket) <0) {
        free_data(data);
        return -1;
    }
    
    return 0; // valid request
}


/*
    Helper function for retrieve_file
    Return the query data based on the target file and range
 */
uint8_t *valid_request_retrieve_file(struct client_data *data) {
    
    /* read payload content */
    // read target file name since 20-th byte
    char *target_file = (char *)&data->payload_msg[TYPEDIGIT * 5];
    // convert 8 bytes to 1 uint64_t
    uint64_t start_offset = parse(&data->payload_msg[TYPEDIGIT], BYTE_SIZE);
    // convert 8 bytes to 1 uint64_t
    uint64_t data_length = parse(&data->payload_msg[TYPEDIGIT * 3], BYTE_SIZE);
    
    uint64_t file_size = 0;
    //Check if file exist and calculate the size of the given 'target_file'
    //Send an error message if the 'target_file' not exits and return NULL
    char *path = check_regular_file(data, target_file, &file_size);
    if (path == NULL) { // target file doesnot exits
        free(path);
        return NULL;
    }
    // check valid range
    uint64_t request_range = start_offset + data_length;
    if (request_range > file_size) {
        error_response(data->new_socket);
        free(path);
        return NULL;
    }
    
    /* valid request */
    FILE *fd = fopen(path, "r");  // read target file
    if (fd == NULL) {
        printf("Fail to read file [%s]\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(fd, start_offset, SEEK_SET); // set read pointer
    uint8_t *file_contents = malloc(data_length); //read all data at once
    fread(file_contents, data_length, 1, fd);
    fclose(fd);
    free(path);
    
    return file_contents;
}


/*
    Send data ti client for retrieve_file
    
    Send response to client
    In this case, different connections have the same sessionID and file range
    Send the respoinse on one connection, others response empty payload
 */
void send_retrive_file_response(struct client_data *data,
                                struct session *sessions, uint32_t sessionID,
                                uint64_t start_offset, uint64_t data_length,
                                char *target_file, uint8_t *file_contents) {
    
    if (data->reqr_compression == false) {
        send_uncompression_msg(data, data_length, file_contents);
    }
    else {
        /* Send "compressed" msg to client*/
        send_compression_msg(data, data_length, sessionID, start_offset,
                             target_file, file_contents);
    }
}

