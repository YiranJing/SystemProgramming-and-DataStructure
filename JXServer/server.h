#ifndef SERVER_H
#define SERVER_H

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

#define MSGINFO (9)
#define BYTE_SIZE (8)
#define TYPEDIGIT (4)
#define INIT_CAPACITY (20)
#define DICT_SIZE (256)
#define COMPRESSION_DIR ("compression.dict")
#define DEFAULT_BUFFER_SIZE (1024)
#define THREAD_POOL_SIZE (20)
#define SMALL_BUFFER (100)



struct server {
    struct in_addr inaddr;
    uint16_t port;
    char *addr; /* IPv4 address in dotted quad notation */
    char *target_dir;
    int socket_fd;
    struct sockaddr_in address;
    pthread_t *pthreads;
    int n_threads;
};

struct client_data {
    struct server *server;
    int new_socket; /*fd of a new request*/
    struct compress_dict *compress_dict; /* compressed directory */
    struct session *sessions; /* array of sessions from same client */
    uint8_t digit_type; /* the first 4 bit in message header */
    uint8_t recv_compression; /* the fifth compression bit in message header */
    uint8_t reqr_compression; /* the sixth compression bit in message header */
    uint8_t payload_length[BYTE_SIZE]; /* 8 byte payload length */
    uint8_t *payload_msg; /* varianle type payload */
    uint64_t payload_int; /* the length of payload as the 64 unsigned byte*/
};


/* payload in retrieve file functionality*/
struct request {
    uint32_t sessionID; /* the first 4 bytes in payload msg */
    char * target_file; /* file name */
    uint64_t start_offset; /* start offset of data*/ //这个格式改成下斜杠
    uint64_t data_length; /* data range */
};


struct session {
    struct request *requests; /* array to record concurrent sessions */
    uint64_t capacity; /* capacity of sessions */
    uint64_t size; /* current length of sessions */
    pthread_mutex_t lock; /* handle multiplex */
};


struct compress_dict *read_compression_dir();

void decode_msg(struct client_data *data);

unsigned char *encode_msg(struct client_data *data, uint8_t *response,
                          int payload_length, int *compress_length);

void encode_data(struct client_data *data);

struct server read_address_and_port(char *file_name);

void *connection_handler(void *arg);

struct server read_address_and_port(char *file_name);

void create_bind_socket(struct server *server, int *option);

int create_new_connected_socket(struct server *server);

void error_response(int new_socket);

int check_error_functionality(uint8_t type, int new_socket);

void echo_handler(struct client_data *data);

void echo(struct client_data *data, uint8_t header);

void dir_list_handler(struct client_data *data, int payload_length,
                      void *response);

void dir_list(struct client_data *data);

void size_query(struct client_data *data);

void retrieve_file(struct client_data *data);




#endif /* server_h */
