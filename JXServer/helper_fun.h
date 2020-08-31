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



void read_message_header(uint8_t header, struct client_data *data);

int binary2decimal(char *n, int size);

uint64_t parse(uint8_t *v, uint8_t size );

void read_payload_length(struct client_data *data, uint8_t *buffer);

uint8_t *read_compression_byte(unsigned long *filelen);

char *check_regular_file(struct client_data *data, char *target_file,
                          uint64_t *file_size);

void send_data(struct client_data *data, uint8_t response_header);

uint8_t *merge_infor(struct client_data *data, uint8_t *file_contents,
                      uint64_t payload_length);

void free_data(struct client_data *data);

void send_empty_payload(struct client_data *data);

struct session *session_init();

int check_sessions(struct client_data *data, struct session *sessions,
                   uint32_t sessionID, uint64_t start_offset,
                   uint64_t data_length, char *target_file);

void remove_session(struct session *sessions, uint32_t sessionID,
                    uint64_t start_offset, uint64_t data_length,
                    char *target_file);

void send_uncompression_msg(struct client_data *data, uint64_t data_length,
                            uint8_t *file_contents);

void send_compression_msg(struct client_data *data, uint64_t data_length,
                          uint32_t sessionID, uint64_t start_offset,
                          char *target_file, uint8_t *file_contents);

struct client_data *client_data_init(struct server *server,
                                     struct compress_dict *compress_dict,
                                     struct session *sessions);

void destroy_dictionary(struct compress_dict *compress_dict);

void destory_all(struct compress_dict *compress_dict,
                 struct session *sessions, struct server *server) ;

int check_valid_request(uint8_t *buffer, struct client_data *data);

uint8_t *valid_request_retrieve_file(struct client_data *data);

void send_retrive_file_response(struct client_data *data,
                                struct session *sessions, uint32_t sessionID,
                                uint64_t start_offset, uint64_t data_length,
                                char *target_file, uint8_t *file_contents);



#endif
