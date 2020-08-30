#ifndef QUEUE_H
#define QUEUE_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct queue_node {
	struct queue_node* next;
	void* data;
};


struct queue {
	struct queue_node* top;
	size_t len;
};


struct queue* queue_init();

void queue_push_copy(struct queue* s, void* data, size_t nbytes);

void queue_free(struct queue* s);

void* queue_pop_object(struct queue* s);

void print_queue(struct queue *s);




#endif
