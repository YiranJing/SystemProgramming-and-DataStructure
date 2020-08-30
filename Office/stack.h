#ifndef STACK_H
#define STACK_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct stack_node {
	struct stack_node* next;
	void* data;
};


struct stack {
	struct stack_node* top;
	size_t len;
};


struct stack* stack_init();

void stack_push_copy(struct stack* s, void* data, size_t nbytes);

void stack_free(struct stack* s);

void* stack_pop_object(struct stack* s);

void print_stack(struct stack *s);


#endif
