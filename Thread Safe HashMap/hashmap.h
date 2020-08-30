#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
//#define init_capacity ((size_t)(0xffff))

#define init_capacity 32

/* hash map is a table of void pointer */

struct hash_map{

  pthread_mutex_t lock; // create mutex object for lock purpose
  size_t size; //size of hashmp
  struct bucket **buckets; // list of dynamic array
  size_t n_buckets; // number of buckets
  int (*cmp)(void*,void*);
  size_t (*hash)(void*);
  void (*key_destruct)(void*);
  void (*value_destruct)(void*);
};


struct bucket{
  pthread_mutex_t lock;
  size_t hash;
  struct node *nodes; // the header of linked list for each bucket
  size_t n_node; // number of node in the linked list
};

//
struct node{
  pthread_mutex_t lock;
  void *k; // key of entry
  void *v; // value of entry
  struct node *next; // a pointer to another node, then can chain them together in case collision occurs
};


struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*,void*),
    void (*key_destruct)(void*), void (*value_destruct)(void*));

void hash_map_put_entry_move(struct hash_map* map, void* k, void* v);

void hash_map_remove_entry(struct hash_map* map, void* k);

void* hash_map_get_value_ref(struct hash_map* map, void* k);

void hash_map_destroy(struct hash_map* map);

void bucket_add_node(struct bucket **bucket, void* k, void* v);

void free_map_bucket(struct hash_map* map);

void* hash_map_get_entry_ref(struct hash_map* map, void* k, struct bucket *bucket);

void hashmapLock(struct hash_map* map);
void hashmapUnlock(struct hash_map* map);
void bucketLock(struct bucket *bucket);
void bucketUnlock(struct bucket *bucket);
void nodeLock(struct node* node);
void nodeUnlock(struct node* node);

#endif
