#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "hashmap.h"


/*
	helper funtion for hash_map_new
*/
void init_buckets(struct bucket **buckets, size_t n_buckets) {
	for (size_t i = 0; i< n_buckets; i++) {
		struct bucket **bucket = &buckets[i];
    *bucket = malloc(sizeof(struct bucket));
		(*bucket)->hash = i; // the hash value is equal to its index
		(*bucket)->n_node = 0;
		(*bucket)->nodes = NULL;

		if (pthread_mutex_init(&((*bucket)->lock), NULL) != 0) {
            perror("unable to initialize bucket->lock mutex");
            return;
    	}
	}
	return;
}



// call only once per hashmap instance
// no need to be thread safe
// The type for key and value can be of any type,
// that is why hash, cmp, key_destruct and value_destruct is provided.
struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*,void*),
    void (*key_destruct)(void*), void (*value_destruct)(void*)) {

	// check valid input
	if (hash == NULL || cmp == NULL || key_destruct == NULL || value_destruct == NULL){
		return NULL;
	}

	// initial capacity of the HashMap is 16
	struct hash_map* map = calloc(1, sizeof(struct hash_map));
	map->size = 0; // initially nothing here
	map->buckets = calloc(init_capacity, sizeof(struct bucket *));
	map->n_buckets = init_capacity;

	// initialise hash table (bucket array)
	init_buckets(map->buckets, map->n_buckets);
  map->cmp = cmp;
	map->hash = hash; // the given hash function
	map->value_destruct = value_destruct;
	map->key_destruct = key_destruct;
	pthread_mutex_init(&(map->lock), NULL);

	return map;
}

// hashIndex = key % noOfBuckets
size_t calculateIndex(size_t n_buckets, size_t hash) {
	return hash % n_buckets;
}



/*
	The capacity of the HashMap is doubled each time it reaches the threshold
	The default load factor of HashMap is 0.75f
*/
void expandIfNecessary(struct hash_map* map){
	//hashmapLock(map);
  map->size++;

	if (map->size <= (map->n_buckets * 3/4) ) {
		//hashmapUnlock(map);
		return;
	}
	size_t new_n_buckets =  map->n_buckets * 2;
	struct bucket **new_bucket = calloc(new_n_buckets, sizeof(struct bucket *));
  init_buckets(new_bucket, new_n_buckets);

	for (size_t i = 0; i<map->n_buckets; i++) {
		struct bucket *bucket = map->buckets[i];
    struct node *n = bucket->nodes;
    while(n!= NULL) {
      struct node *next = n->next;
			int hash = map->hash(n->k);
			size_t index = calculateIndex(new_n_buckets, hash); // new index
			struct bucket **new_current_buccket = &new_bucket[index];
			bucket_add_node(new_current_buccket, n->k, n->v);
			n = next; // go to the next node
		}
	}
  free_map_bucket(map);
  map->buckets = new_bucket;
  map->n_buckets = new_n_buckets;
  //hashmapUnlock(map);
}



void hashmapLock(struct hash_map* map) {
    pthread_mutex_lock(&map->lock);
}
void hashmapUnlock(struct hash_map* map) {
    pthread_mutex_unlock(&map->lock);
}

void bucketLock(struct bucket *bucket){
	pthread_mutex_lock(&bucket->lock);
}

void bucketUnlock(struct bucket *bucket){
	pthread_mutex_unlock(&bucket->lock);
}

void nodeLock(struct node* node) {
  pthread_mutex_lock(&node->lock);
}

void nodeUnlock(struct node* node) {
  pthread_mutex_unlock(&node->lock);
}



/*
	helper function of hash_map_put_entry_move
*/
void bucket_add_node(struct bucket **bucket_ptr, void* k, void* v){

  struct bucket *bucket = *bucket_ptr;
  bucket->n_node++; // increase the number of nodes stored in current linkedlist

	if (bucket->nodes == NULL) {

		bucket->nodes = calloc(1, sizeof(struct node));
		bucket->nodes->v = v;
		bucket->nodes->k = k;
		bucket->nodes->next = NULL;
		pthread_mutex_init(&(bucket->nodes->lock), NULL);
	}
	else {
		// find the last node in linkedlist
		struct node *cursor = bucket->nodes;
		while (cursor->next != NULL) {
			cursor = cursor->next;
		}
		cursor->next = calloc(1,  sizeof(struct node));
    cursor->next->v = v;
		cursor->next->k = k;
		cursor->next->next = NULL;
    pthread_mutex_init(&(cursor->next->lock), NULL);
	}
}



/*
	Puts an entry to the hashmap, associating a key with value
	move the memory given to the hashmap
*/
void hash_map_put_entry_move(struct hash_map* map, void* k, void* v) {

	size_t hash = map->hash(k);
	// invlid input
	if (k == NULL || v == NULL) {
		return;
	}

  hashmapLock(map);
  size_t index = calculateIndex(map->n_buckets, hash);
	struct bucket **current_bucket = &map->buckets[index];
  struct bucket *bucket = *current_bucket;
  //bucketLock(bucket);
  //hashmapUnlock(map);

  /* check if the key exists already */
  struct node *n = bucket->nodes;
	for (size_t j = 0; j<bucket->n_node; j++) {
			size_t equal = map->cmp(n->k, k);
			if (equal == 1) {
			  // if the key exist
				// old entry should be removed (hash_map_remove_entry)
				map->value_destruct(n->v);
        map->value_destruct(n->k);
        n->k= k;
		    n->v = v; // resign the new value
				//bucketUnlock(bucket);
                hashmapUnlock(map);
				return;
			}
      n = n->next;
	}
  /* key is not exists, a new entry should be created */
	bucket_add_node(current_bucket, k, v);
	//bucketUnlock(bucket);
	expandIfNecessary(map);
    hashmapUnlock(map);
}






/*
  Return the entry reference of the given k
*/
void* hash_map_get_entry_ref(struct hash_map* map, void* k, struct bucket *bucket){

  struct node *n = bucket->nodes;
	for (size_t j = 0; j<bucket->n_node; j++) {
			int equal = map->cmp(n->k, k); // error!
			if (equal == 1) {
				return n;
			}
      n = n->next;
	}
	return NULL;
}





void hash_map_remove_entry(struct hash_map* map, void* k) {

	if (k == NULL) {
    return;
  }
   hashmapLock(map);
    
  size_t hash = map->hash(k);
	size_t index = calculateIndex(map->n_buckets, hash);
	// get the current bucket
	struct bucket *curr_bucket = map->buckets[index];

	//bucketLock(curr_bucket);
  struct node *node = hash_map_get_entry_ref(map, k, curr_bucket);
  if (node == NULL) {
    //bucketUnlock(curr_bucket);
      hashmapUnlock(map);
    return;
  }

  struct node *n = curr_bucket->nodes;
  size_t c = map->cmp(n->k, k);

  if (c == 1) {
    struct node *n_next = n->next;
    map->size--;
    curr_bucket->n_node--;
    map->key_destruct(node->k);
  	map->value_destruct(node->v);
		free(node);
		curr_bucket->nodes = n_next;
	}
	else {
		while (map->cmp(n->next->k, k) != 1 && n->next->k != NULL) {
			n = n->next;
		}
		n->next = node->next;
    map->size--;
    curr_bucket->n_node--;
    map->key_destruct(node->k);
  	map->value_destruct(node->v);
		free(node);
	}
	//bucketUnlock(curr_bucket);
    hashmapUnlock(map);
}




/*
	retrive an entry based on the key within the map itself
	it should return a reference of that key
  Return the value of the entry
*/
void* hash_map_get_value_ref(struct hash_map* map, void* k) {

	if (k == NULL) {
		return NULL;
	}
  size_t hash = map->hash(k); // calculate hash value
  //hashmapLock(map);
  // check valid input
  struct node * node_ref = NULL;
	size_t index = calculateIndex(map->n_buckets, hash);
	// get the current bucket
	struct bucket *bucket = map->buckets[index];
		//bucketLock(bucket);
    struct node *n = bucket->nodes;
		for (size_t j = 0; j<bucket->n_node; j++) {

			int equal = map->cmp(n->k, k);
			if (equal == 1) {
				node_ref =  n;
        break;
			}
      n = n->next;
		}
	  //bucketUnlock(bucket);

	if (node_ref == NULL) {
        //hashmapUnlock(map);
		return NULL;
	}
    //hashmapUnlock(map);
  return node_ref->v;
}




void node_list_destroy(struct node *n, void (*key_destruct)(void*), void (*value_destruct)(void*)){
	struct node * temp = NULL;

	while(n != NULL) {
		temp = n->next;
		key_destruct(n->k);
	  value_destruct(n->v);
		pthread_mutex_destroy(&(n->lock));
		free(n);
		n = temp;
	}
	free(n); // free the last node
}



// call only once per hashmap instance
void hash_map_destroy(struct hash_map* map) {

	size_t n_buckets = map->n_buckets;

	for (size_t i = 0; i<n_buckets; i++) {
		struct bucket *bucket = map->buckets[i];
		if (bucket->n_node != 0) { // not empty bucket
			struct node *head = bucket->nodes;
			node_list_destroy(head, map->key_destruct, map->value_destruct);
		}
		pthread_mutex_destroy(&(bucket->lock));
		free(bucket);
	}
    free(map->buckets);
	pthread_mutex_destroy(&(map->lock));
	free(map);
}



void node_list_free(struct node *n){
	struct node * temp = NULL;

	while(n != NULL) {
		temp = n->next;
		pthread_mutex_destroy(&(n->lock));
		free(n);
		n = temp;
	}
	free(n); // free the last node
}



// free buckect only, not memory, and not destruct key or value
void free_map_bucket(struct hash_map* map) {
	// free node on each bucket
	size_t n_buckets = map->n_buckets;

	for (size_t i = 0; i<n_buckets; i++) {
		struct bucket *bucket = map->buckets[i];

		if (bucket->n_node != 0) { // not empty bucket
			struct node *head = bucket->nodes;
      node_list_free(head);
		}
    pthread_mutex_destroy(&(bucket->lock));
		free(bucket);
	}
	free(map->buckets);
}
