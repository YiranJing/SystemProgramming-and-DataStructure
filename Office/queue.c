#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"



/**********************************************************
** building a queue data structure
**********************************************************/



struct queue* queue_init() {
	struct queue* s = malloc(sizeof(struct queue));
	s->top = NULL;
	s->len = 0;
	return s;
}

void queue_push_copy(struct queue* s, void* data, size_t nbytes) {

	if(s == NULL || data == NULL || nbytes == 0) { return; };

  // 1. Create a new item and set its value
	struct queue_node* new_node = malloc(sizeof(struct queue_node));
  //puts("create queue node line 27 queue.c");
  new_node->data = data;

  new_node->next = NULL;
	struct queue_node* current_top = s->top;
	s->top = new_node;
	s->top->next = current_top;
	s->len++; // increase queue length by one

}

void queue_free(struct queue* s) {
	if(s != NULL) {
		void* n = NULL;

    for (size_t i = 0; i< s->len; i++){
      n = queue_pop_object(s);
    }
	}
	free(s);
}


void* queue_pop_object(struct queue* s) {

	if (s == NULL){ // check empty list
    //printf("cannot pop null\n");
		return NULL;
	}

  if (s->top == NULL) {
    return NULL;
  }

	// 1. Iterate to the last node
	struct queue_node *p = s->top;

  if (s->len == 1){ // the == 1 last node
    void* d = p->data;
    //puts("free first node line 70 queue.c");
    free(p);
    s->len--; // decrease length
    s->top = NULL;
	  return d;
  }

  //struct queue_node* last_node = malloc(sizeof(struct queue_node));

  //while (p->next->next != NULL){ // READ unknown memory access
    //p = p->next;
  //}
  for (size_t i=2; i<(s->len); i++){
    p = p->next;
  }

  struct queue_node* last_node = p->next;
  p->next = NULL;
  void* d = last_node->data;
  //puts("free last_node line 89 queue.c");
  free(last_node);
	s->len--; // decrease length
	return d;
}


void print_queue(struct queue *s){

	/*
	// empty list check
	if (s->top == NULL){
		printf("Enpty queue\n");
		return;
	}
	// guaranteed no empty list
	struct queue_node *p = s->top;

	while(p!= NULL){
		struct employee* d = (struct employee*) p->data;
		printf("%s\n", d->name);
		p = p->next;
	}
	puts("---------------- queue end ----------------------");
	*/
}
