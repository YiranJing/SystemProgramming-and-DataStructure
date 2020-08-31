#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


struct node_tree {
  uint8_t data;
  int is_leaf;
  struct node_tree *left;
  struct node_tree *right;
};

struct binary_tree {
    struct node_tree *root;
};

struct compress_dict {
    struct binary_tree *tree;
    uint8_t dict_bit_array[DICT_SIZE * 4]; /* bit array for compression infor */
    int seg_index[DICT_SIZE + 1]; /* start index of 256 segments and padding */
};



void set_bit(uint8_t *array, int index);

void clear_bit(uint8_t *array, int index);

uint8_t get_bit(uint8_t *array, int index);

struct binary_tree *tree_init();

void tree_destroy(struct node_tree *root);

struct node_tree *create_node(int is_leaf, int value);

struct node_tree *insert_node(struct node_tree *root, struct node_tree *node,
                              char direction);


#endif
