#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"
/*
	We will be building a stack data structure
	This will be used push and pop data from the stack
	and will be backed by a linked list.

*/



/**********************************************************
** building a stack data structure
**********************************************************/

struct stack* stack_init() {
	struct stack* s = malloc(sizeof(struct stack));
	s->top = NULL;
	s->len = 0;
	return s;
}


void stack_push_copy(struct stack* s, void* data, size_t nbytes) {

	if(s == NULL || data == NULL || nbytes == 0) { return; };

	struct stack_node* new_node = malloc(sizeof(struct stack_node));
	new_node->data = data;
	new_node->next = NULL;
	struct stack_node* current_top = s->top;
	s->top = new_node;
	s->top->next = current_top;

}

void stack_free(struct stack* s) {
	if(s != NULL) {
		void* n = NULL;
		while( (n = stack_pop_object(s)) != NULL) {
			free(n);
		}
	}
	free(s);
}


void* stack_pop_object(struct stack* s) {
	if(s != NULL) {
		//Not checking if top == NULL
		if(s->top != NULL) {
			struct stack_node* node = s->top;
			s->top = s->top->next;
			void* d = node->data;
			free(node);
			return d;
		}

	}
	return NULL;
}


void print_stack(struct stack *s){
	/*
	// empty list check
	if (s->top == NULL){
		printf("Enpty stack\n");
		return;
	}
	// guaranteed no empty list
	struct stack_node *p = s->top;

	while(p!= NULL){
		struct employee* d = (struct employee*) p->data;
		printf("%s\n", d->name);
		p = p->next;
	}
	puts("---------------- stack end ----------------------");
	*/
}
