#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "unrolled.h"

/* Create an empty unrolled linked list on heap */
struct unrolled_ll* unrolled_ll_new(size_t n) {

    // check valid input
    if (n == 0) {
        return NULL;
    }
    
    struct unrolled_ll* list = calloc(1, sizeof(struct unrolled_ll));
    list->n = n;
    
    if(pthread_mutex_init(&list->lock, NULL) != 0){
        perror("unable to initialise mutex");
        return NULL;
    }
    return list;
}

void create_new_node(struct unrolled_ll* list, struct node *node, void *element){

    struct node *new_node = calloc(1, sizeof(struct node)); // create new node
    
    new_node->elements = calloc(list->n, sizeof(void *)); 
    
    if(pthread_mutex_init(&new_node->lock, NULL) != 0){
            perror("unable to initialise mutex");
            return;
    }
    
    new_node->elements[0] = element; // add the element as the first element on the new node
    new_node->next = NULL;
    new_node->capacity++;
    list->len++; // increase the number of elements
    list->n_node++; // increase the number of nodes
    node->next = new_node;
}


void unrolled_ll_append(struct unrolled_ll* list, void* element) {
    
    pthread_mutex_lock(&list->lock);
    
    // Case 1: empty list
    if (list->head == NULL) {

        list->head = calloc(1, sizeof(struct node)); // create new node
        list->len = 1;
        list->n_node = 1;

        list->head->elements = calloc(list->n, sizeof(void *)); /// create new array with length n
        if(pthread_mutex_init(&list->head->lock, NULL) != 0){
            perror("unable to initialise mutex");
            return;
        }
        list->head->elements[0] = element; // add the element as the first element on the new node
        list->head->next = NULL;
        list->head->capacity = 1; // only one node

        pthread_mutex_unlock(&list->lock);
        return;
    }

    struct node * node = list->head;

    // get the last node of the list
    for(size_t i = 1; i<list->n_node; i++){
        node = node->next;
    }
    // check the capacity of the last node
    if (node->capacity < list->n) {
        node->elements[node->capacity] = element;
        node->capacity++;
        list->len++;
    }
    else { // create new node 
       create_new_node(list, node, element);
    }
    pthread_mutex_unlock(&list->lock);
    return;
}

void* unrolled_ll_remove(struct unrolled_ll* list, size_t i) {

    pthread_mutex_lock(&list->lock);
    // check valid input
    if (i >= (list->len)) {
        pthread_mutex_unlock(&list->lock);
        return NULL;
    }
    void * result = NULL;
    struct node * node = list->head;
    struct node * node_parent = NULL; // save the node's parent, which is null if the node is head
    size_t pos = 0; // the number of elements we have checked
    while(node != NULL) {
        for (size_t j = 0; j<list->n; j++) {
            void *element = node->elements[j];
            if (element != NULL){  
                if (pos == i) {   // find the element based on the index
                    result = node->elements[j];
                    node->elements[j] = NULL; // remove the node 
                    list->len--; // decreate one element on the whole list
                    // check empty node 
                    for (size_t k = 0; k<list->n; k++) {
                        void *temp = node->elements[k];
                        if (temp!= NULL){ 
                            pthread_mutex_unlock(&list->lock);
                            return result; // no-empty node
                        }
                    }
                    // empty list
                    if (list->len == 0) {
                        list->n_node--;
                        pthread_mutex_unlock(&list->lock);
                        return result;
                    }
                    node_parent->next = node->next; 
                    free(node->elements);
                    free(node);
                    list->n_node--;
                    pthread_mutex_unlock(&list->lock);
                    return result;
                }
                pos++;
            }
        }
        //go to the next node
        node_parent = node;
        node = node->next;
    }
    pthread_mutex_unlock(&list->lock);
    return NULL;
}


void* unrolled_ll_get(struct unrolled_ll* list, size_t i) {
    
    // check valid input
    if (i >= (list->len)) {
        return NULL;
    }
    pthread_mutex_lock(&list->lock);

    void* result = NULL;
    struct node * node = list->head;
    size_t pos = 0; // the number of elements we have checked
    while(node != NULL) {
        for (size_t j = 0; j<list->n; j++) {
            void *element = node->elements[j];
            if (element != NULL){  
                if (pos == i){ // find the element based on the index
                    pthread_mutex_unlock(&list->lock);
                    return element;
                }
                pos++;
            }
        }
        node = node->next;
    }
    pthread_mutex_unlock(&list->lock);
    return result;
}

void unrolled_ll_destroy(struct unrolled_ll* list) {

    struct node * n = list->head;
    struct node * temp = NULL;

    while(n != NULL) {
        temp = n->next;
        pthread_mutex_destroy(&n->lock);
        free(n->elements);
        free(n);
        n = temp;
    }
    pthread_mutex_destroy(&list->lock);
    free(list);
}


