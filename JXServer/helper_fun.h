#ifndef helper_fun_h
#define helper_fun_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <endian.h>
#include <unistd.h>
#include <ctype.h>

#define MSGINFO 9
#define TYPEDIGIT 4
#define THREAD_POOL_SIZE 20


void read_message_header(uint8_t header, struct client_data *data);

int binary2decimal(char *n, int size);

uint64_t parse(uint8_t *v, uint8_t size );

void read_payload_length(struct client_data *data, uint8_t *buffer);

char * check_regular_file(struct client_data * data, char * target_file, uint64_t *file_size);












#endif
