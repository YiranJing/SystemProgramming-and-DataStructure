#ifndef UNROLLED_H
#define UNROLLED_H

#include <stdlib.h>
#include <pthread.h>


// clang -g -Wall -Werror -std=gnu11 unrolled.c -o Q2 -lpthread -fsanitize=address

struct unrolled_ll{
    struct node *head; /* the first node of unrolled linked list */
    size_t len; /* the number of not-null element in the whole linked list */
    size_t n_node; /* the number of nodes in this linked list */
    size_t n; /* the max number of elements in each node */
    pthread_mutex_t lock;
};

struct node{
    void **elements; /* array with length = n */
    struct node *next; 
    pthread_mutex_t lock;
    size_t capacity; /* number of elements each node can contain*/ 
};

// Case 1: empty list

// Case 2: single element list

// Case 3: problems at head 

// Case 4: problems at end

// Case 5: noral case 

struct unrolled_ll* unrolled_ll_new(size_t n);

void unrolled_ll_append(struct unrolled_ll* list, void* element);

void* unrolled_ll_remove(struct unrolled_ll* list, size_t i);

void* unrolled_ll_get(struct unrolled_ll* list, size_t i);

void unrolled_ll_destroy(struct unrolled_ll* list);

#endif
