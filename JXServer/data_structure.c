#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <endian.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#include "server.h"
#include "helper_fun.h"
#include "data_structure.h"



/* USYD CODE CITATION ACKNOWLEDGEMENT
* I declare that the majority of the following function has been copied from
* mathcs emory website with only minor changes and it is not my own work.
*
* http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
*/
void set_bit(uint8_t* array, int index){
    
    int i = index/8; // index within bit array
    int pos = index%8;

    uint8_t flag = 1;

    flag = flag << (8-pos-1); // (BYTE_SIZE - 1 - x % BYTE_SIZE)
    array[i] = array[i] | flag;
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
* I declare that the majority of the following function has been copied from
* mathcs emory website with only minor changes and it is not my own work.
*
* http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
*/
void clear_bit(uint8_t *array, int index) {
    int i = index / 8;
    int pos = index % 8;

    unsigned int flag = 1;

    flag = flag << (8 - pos - 1);
    flag = ~flag;

    array[i] = array[i] & flag;
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
* I declare that the majority of the following function has been copied from
* sanfoundry website with only minor changes and it is not my own work.
*
* https://www.sanfoundry.com/c-program-implement-bit-array
*/
uint8_t get_bit(uint8_t* array, int index) {
    
    uint8_t result = 0x0;
    int i = index / 8;
    int pos = index % 8;
    uint8_t flag = 1;
    flag = flag << (8 - pos - 1);
    
    if ((array[i] & flag) != 0)
        result = 0x1;
    return result;
}



/*
    Initialise binary tree with empty root
 */
struct binary_tree *tree_init() {
    struct binary_tree *tree = calloc(1, sizeof(struct binary_tree));
    struct node_tree *node = create_node(0, 0); // root of binary tree
    tree->root = node;
    return tree;
}


/*
    Create new node
    If is_leaf is true, add segment value to it.
 */
struct node_tree *create_node(int is_leaf, int value) {
    struct node_tree *tree_node = malloc(sizeof(struct node_tree));
    tree_node->is_leaf = is_leaf;
    if (is_leaf) {
        tree_node->data = value;
        //printf("add leaf node %u \n", tree_node->data);
    }
    tree_node->left = NULL;
    tree_node->right = NULL;
    return tree_node;
}


/*
    Insert node based on the direction given
    Go to right if direction = 1, otherwise, direction = 0, go to left
    Add new node if it doesnot exist in the binry tree
 
    Return the position of node
 */
struct node_tree *insert_node(struct node_tree *root,
                              struct node_tree *node, char direction) {
    
    struct node_tree *cursor = NULL;
    
    if (direction) { // Go to right if direction = 1
        if (root->right == NULL)
            root->right = node;
        cursor = root->right;
    }
    else { // go to left
        if (root->left == NULL)
            root->left = node;
        cursor = root->left;
    }
    return cursor;
}


/*
    Destrory binary tree, free all associated memory
 */
void tree_destroy(struct node_tree *root) {
    if (root == NULL)
        return;
    tree_destroy(root->right); // destory subtree
    tree_destroy(root->left);
    free(root);
}


