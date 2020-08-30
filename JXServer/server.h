#ifndef server_h
#define server_h

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

#define MSGINFO 9
#define TYPEDIGIT 4
#define THREAD_POOL_SIZE 20
#define COMPRESSION_DIR ("compression.dict")


struct node_tree {
  uint16_t content;
  struct node_tree *left;
  struct node_tree *right;
  char is_leaf;
};

struct binary_tree {
    struct node_tree *root;
};

struct compress_dict{  // store informtion of compression.dict
    struct binary_tree *tree;
    //uint8_t *dict;
    //int *len;
};


struct server {
    struct in_addr inaddr;
    uint16_t port;
    char * addr; /* IPv4 address in dotted quad notation */
    char * target_dir;
    int socket_fd;
    struct sockaddr_in address;
    pthread_t thread_pool[THREAD_POOL_SIZE];
};

struct client_data {
    struct server * server;
    int new_socket;
    char digit_type; /* convert from the first 4 bit in the message header */
    char recv_compression;  /* the fifth compression bit in the message header*/
    char reqr_compression; /* the sixth compression bit in the message header*/
    uint8_t payload_length[MSGINFO-1]; /* 8 byte payload length */
    uint8_t *payload_msg; /* varianle type payload */
    uint64_t payload_int; /* the length of payload as the 64 unsigned byte*/
};


void *connection_handler(void* arg);

struct server read_address_and_port(char *file_name);

void create_bind_socket(struct server *server, int *option);

int create_new_connected_socket(struct server * server);

void error_response(int new_socket);

int check_error_functionality(char check, int new_socket);

void free_data(struct client_data *data);

void echo(struct client_data * data, uint8_t header);

void dir_list(struct client_data * data);

void size_query(struct client_data * data);

void retrieve_file(struct client_data * data);


#endif /* server_h */
