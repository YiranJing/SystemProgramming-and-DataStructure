#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
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
#include "huffman.h"


/*
    Function for bit array
*/
void set_bit(uint8_t* array, int index){
    int i = index / 8; // index within bit array
    int pos = index % 8;
    array[i] = array[i] | 1 << (8-pos-1);
}

/*
    Function for bit array
*/
void clear_bit(uint8_t *array, int index) {
    int i = index / 8;
    int pos = index % 8;
    array[i] = array[i] & ~(1 << (8 - pos - 1));
}

/*
    Function for bit array
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
    Function for huffman tree
*/
/*
    Initialise tree with 256 nodes
    each nodes for one unique symbol:
        key: 0-255
        value: the frequecy of this symbol in the target file
*/
struct huff_tree* tree_init(uint8_t *key, uint64_t *freq, int size) {
    
    struct huff_tree* huff_tree = calloc(1, sizeof(struct huff_tree));
    // create 256 nodes for each symbol
    // the key and frequency of each node is based on the given target file
    for (int i = 0; i < size; i++) {
        struct tree_node* new_node = calloc(1, sizeof(struct tree_node));
        new_node->key = key[i];
        new_node->freq = freq[i]; // the original freq is sorted by increasing order
        huff_tree->nodes[i] = new_node;
    }
    huff_tree->size = size;
    return huff_tree;
}


/*
    Update binary tree after delete the node with mini frequency
    Swap if the node is a non-leaf node and greater than any of its child
    The new parent node has lower frequency than its two children
*/
void update_tree(struct huff_tree* huff_tree) {
    uint64_t minimum = 0; // index of root
    uint64_t left = 1; // index of the left child
    uint64_t right = 2; // index of the right child
    update_tree_helper(huff_tree, left, right, minimum);
}

/*
    helper function for update_tree
*/
void update_tree_helper(struct huff_tree* huff_tree, uint64_t left,
                        uint64_t right, uint64_t i) {
    
    uint64_t minimum = i;
    /* checking for the smallest element */
    if (right < huff_tree->size) {
        if (huff_tree->nodes[right]->freq < huff_tree->nodes[minimum]->freq) {
            minimum = right;
        }
    }
    if (left < huff_tree->size) {
        if (huff_tree->nodes[left]->freq < huff_tree->nodes[minimum]->freq) {
            minimum = left;
        }
    }
    // parent is greater than at least one child
    if (minimum != i) {
        // exchange parent and child nodes
        struct tree_node* temp = huff_tree->nodes[minimum];
        huff_tree->nodes[minimum] = huff_tree->nodes[i];
        huff_tree->nodes[i] = temp;
        
        // continue update the parent
        left = 2 * minimum + 1; // index of the left child is 2k+1
        right = 2 * minimum + 2; // index of the right child is 2k+2
        update_tree_helper(huff_tree, left, right, minimum);
    }
}


  
/*
    Get node with lowest frequency (the root of binary tree)
*/
struct tree_node* get_mini_freq_node(struct huff_tree* huff_tree) {
    struct tree_node* temp = huff_tree->nodes[0]; // the mini value is the root
    // Move the last element to root
    huff_tree->nodes[0] = huff_tree->nodes[huff_tree->size - 1];
    huff_tree->size--;
    update_tree(huff_tree);
    return temp;
}

  
/*
    Create and inster a new node to the huff tree
*/
void insert_huff_tree(struct huff_tree* huff_tree, struct tree_node *right,
                      struct tree_node *left, uint64_t sum) {
    
    struct tree_node *new_node = calloc(1, sizeof(struct tree_node));
    new_node->key = 1; // the key of middle nodes doesnot matter
    new_node->freq = sum; // the freq of the middle nodes is the sum of children's freq
    new_node->left = left;
    new_node->right = right;

    int i = huff_tree->size;
    // binary seach to find the location to insert the new node
    struct tree_node *parent = huff_tree->nodes[(i - 1) / 2];
    while (new_node->freq < parent->freq) {
        // swap if parent's frequency is larger than the new node.
        huff_tree->nodes[i] = parent;
        if (((i - 1) / 2) == 0)
            break;
        i = (i - 1) / 2;
        parent = huff_tree->nodes[(i - 1) / 2];
    }
    huff_tree->size++;
    huff_tree->nodes[i] = new_node;
}
  

/*
    Build a huffman tree based on trecursion
*/
struct tree_node* create_huffman_tree(uint8_t *key, uint64_t *freq, int size) {
    
    struct huff_tree* huff_tree = tree_init(key, freq, size);

    // recurtively build huff tree until only one node left
    while (huff_tree->size > 1) {
        
        // pop two nodes with smallest frequency
        // the left child has higher value than the right child
        struct tree_node *right = get_mini_freq_node(huff_tree);
        struct tree_node *left = get_mini_freq_node(huff_tree);

        printf("right->freq %lu, left->freq %lu \n", right->freq, left->freq);
        //assert(right->freq <= left->freq);
        

        // create a new node and insert to the binary tree
        uint64_t sum = left->freq + right->freq; // the frequency of the new node
        insert_huff_tree(huff_tree, right, left, sum); // insert the new node to the Heap
    }

    // the last node is the root of binary tree
    struct tree_node *root = huff_tree->nodes[0];
    free(huff_tree);
    return root;
}





// helper function for the qsort
// sort based on frequency
int compare(const void * a, const void * b) {
  struct  freq_node * A = (struct  freq_node *)a;
  struct  freq_node * B = (struct  freq_node *)b;
  return ( A->value - B->value );
}


uint8_t *read_file_byte(char *file, uint64_t *filelen) {

    FILE *fileptr;
    uint8_t *buffer;
    fileptr = fopen(file, "r"); // read binary file

    if (fileptr == NULL) {
        perror("unable to open file");
        exit(0);
    }

    // get file size
    fseek(fileptr, 0, SEEK_END);
    *filelen = ftell(fileptr);
    rewind(fileptr); // sets the file position to the beginning of the file

    buffer = malloc((*filelen) * sizeof(uint8_t));
    fread(buffer, 1, *filelen, fileptr); // read and store all bytes to buffer
    fclose(fileptr);
    return buffer;
}


// Function to find counts of all elements in the array
uint64_t * calculate_frequency(uint8_t *arr, uint64_t n, uint64_t *freq) {
  
    for (uint64_t i = 0; i<n; i++)
        freq[arr[i]] += 1; //update the frequency of array[i]
    return freq;
}


/*
    Generate node list and sort in increasing order
    Saved in dict->nodes
*/
void generate_node_list(struct compress_dict *dict) {
   
    dict->nodes = calloc(DICT_SIZE, sizeof(struct freq_node));
    // insert node
    for (uint64_t i=0; i<DICT_SIZE; i++) {
        dict->nodes[i].value = dict->freq[i];
        dict->nodes[i].key = i;
    }
}



/*
    Get the frequency of occurence of the symbol in the input data based on its index
    Print the result on standard output
*/
void get_frequency(struct compress_dict *dict, int index) {
    fprintf(stdout, "%lu", dict->freq[index]);
}


/*
    check option value is valid (from 0 to 255)
    exist if invalid option value, otherwise return the number
*/
int get_value(char *option) {
    int value = atoi(option); // a byte value from 0-255
    if (value < 0 || value >255)// invalid input
        exit(0);
    return value;
}


void save_bit_array(int *arr, int n, uint8_t *bit_array) {
    int i;
    for (i = 0; i < n; i++) {
        if (arr[i] == 1) {
            set_bit(bit_array, i);
        }
    }
}

void save_codes(struct tree_node* root, int* arr, int top,
                struct compress_dict *dict) {
    // 0 to right edge
    if (root->right) {
        arr[top] = 0;
        save_codes(root->right, arr, top + 1, dict);
    }
    // 1 to left edge
    if (root->left) {
        arr[top] = 1;
        save_codes(root->left, arr, top + 1, dict);
    }
    // the node has no child is the leaf
    int is_leaf = (root->left == false) && (root->right == false);
    if (is_leaf) {
        // get node based on the key
        struct freq_node * node = &dict->nodes[root->key]; // (key - 1) depth
        node->length = top;
        // calculate padding
        node->n_padding = (8 - top%8);
        if (node->n_padding == 8) {
            node->n_padding = 0;
        }
        save_bit_array(arr, top, node->bit_array);
    }
}


/*
    Destrory binary tree, free all associated memory
 */
void tree_destroy(struct tree_node* root) {
    if (root == NULL)
        return;
    tree_destroy(root->right); // destory subtree
    tree_destroy(root->left);
    free(root);
}


/*
    build huffman tree using the given data
*/
void calculate_bitcode(struct compress_dict *dict){

    // sort based on frequency in the increasing order
    struct freq_node *sorted_nodes = calloc(DICT_SIZE, sizeof(struct freq_node));
    
    memcpy (sorted_nodes, dict->nodes, DICT_SIZE * sizeof(struct freq_node));
    qsort(sorted_nodes, DICT_SIZE, sizeof(struct freq_node), compare);

    uint8_t *arr = calloc(DICT_SIZE, sizeof(uint8_t));
    uint64_t *freq = calloc(DICT_SIZE, sizeof(uint64_t));
    for (int j = 0; j<DICT_SIZE; j++){
        struct freq_node * node = &sorted_nodes[j];
        arr[j] = node->key;
        freq[j] = node->value;
    };
    
    // apply huff algo
    dict->root = create_huffman_tree(arr, freq, DICT_SIZE);

    // save bitcode for each node
    int arr2[DICT_SIZE] = {0};
    int top = 0;
    save_codes(dict->root, arr2, top, dict);

    tree_destroy(dict->root);
    free(sorted_nodes);
    free(arr);
    free(freq);
}



/*
    Print to standard output if option = 0
*/
void get_bitcode(struct compress_dict *dict, int index) {
    struct freq_node * node = &dict->nodes[index];

    for (uint8_t i = 0; i < node->length; i++) {
        if (get_bit(node->bit_array, i) == 1) {
            printf("%d", 1);
        }
        else{
            printf("%d", 0);
        }
    }
}


/*
    compression the msg in the target file
*/
unsigned char *encode_msg(struct compress_dict *dict, uint8_t *buffer,
                          uint64_t filelen) {

    int number_bit = 0;
    int compress_len = 1; // length of compressed data
    unsigned char *compression_message = malloc(compress_len);
    // iteration: encode each byte
    for (int i = 0; i < filelen; i++) {
        // get one segment from paylaod msg
        struct freq_node * node = &dict->nodes[buffer[i]];

        for (int j = 0; j < node->length; j++) {
            if (number_bit == compress_len * 8){
                compress_len++;
                compression_message = realloc(compression_message,
                                              compress_len);
            }
            // modify compressed msg based on compressed directory
            if (get_bit(node->bit_array, j) == 1){
                set_bit(compression_message, number_bit++);
            } else{
                clear_bit(compression_message, number_bit++);
            }
        }
    }
    // append 0 padding to the compressed msg
    char padding_num = abs(number_bit - compress_len * 8);
    for (int i = number_bit; i  < compress_len * 8; i++) {
        clear_bit(compression_message, i);
    }
    compression_message = realloc(compression_message, ++compress_len);
    // the last byte is the number of paddings in the msg
    compression_message[compress_len - 1] = padding_num;
    dict->compress_len = compress_len;
    return compression_message;
}


unsigned char *create_dictionary(struct compress_dict *dict) {

    int number_bit = 0;
    int compress_len = 1; // length of compressed data
    unsigned char *compression_message = calloc(1, compress_len);
    // iteration: encode each byte
    for (int i = 0; i < DICT_SIZE; i++) {
        
        // get one segment from paylaod msg
        struct freq_node * node = &dict->nodes[i];

        // add bit code length (the length of one segment)
        for (int j = 0; j < 8; j++){
            
            //resize compression_message
            if (number_bit == compress_len * 8) {
                compress_len++;
                compression_message = realloc(compression_message,
                                              compress_len);
            }
            // modify compressed msg
            if (get_bit(&node->length, j) == 1){
                set_bit(compression_message, number_bit++);
            }
            
            else {
                clear_bit(compression_message, number_bit++);
            }
        }
        // add bitcode
        for (int j = 0; j < node->length; j++) {
            
            //resize compression_message
            if (number_bit == compress_len * 8) {
                compress_len++;
                compression_message = realloc(compression_message,
                                              compress_len);
            }

            // modify compressed msg based on compressed directory
            if (get_bit(node->bit_array, j) == 1){
                set_bit(compression_message, number_bit++);
            } else{
                clear_bit(compression_message, number_bit++);
            }
        }
    }
    // append 0 padding to the compressed msg
    char padding_num = abs(number_bit - compress_len * 8);
    for (int i = number_bit; i  < compress_len * 8; i++) {
        clear_bit(compression_message, i);
    }
    compression_message = realloc(compression_message, ++compress_len);
    // the last byte is the number of paddings in the msg
    compression_message[compress_len - 1] = padding_num;
    dict->compress_len = compress_len;
    return compression_message;
}


void write_msg_to_file(struct compress_dict *dict,
                       unsigned char *compression_message, char *option) {


    
    printf("\n\nencode msg: len is %d \n", dict->compress_len);
    for (int i = 0; i<dict->compress_len * 8; i++) {
        if (get_bit(compression_message, i) == 1){
            printf("%d", 1);
        }
        else {
            printf("%d", 0);
        }
    }
    puts("");
    

    FILE *write_ptr = fopen(option,"wb");  // w for write, b for binary
    fwrite(compression_message,dict->compress_len, 1 , write_ptr);
}


int main(int argc, char **argv) {
    // Usage: ./huffman <input file> <command> <option>
    assert(argc == 4);

    char *file = argv[1];
    char *command = argv[2];
    char *option = argv[3];
    // read target file as unsigned char array to buffer
    uint64_t filelen = 0;
    uint8_t *buffer = read_file_byte(file, &filelen);
    struct compress_dict *dict = calloc(1, sizeof(struct compress_dict));
    // calculate the count of each symbol in the target file
    // saved in dict->freq array
    calculate_frequency(buffer, filelen, dict->freq);
    // generate node list and sort in increasing order
    // saved in dict->nodes;
    generate_node_list(dict);
    // calculate bitcode, save binary tree in dict->root
    calculate_bitcode(dict);
   
    /* Frequency */
    if (strcmp(command, "frequency") == 0 ) {
        
        // check option value from 0 to 255
        int value = get_value(option);
        // print the frequency based in the given value(0-255)
        get_frequency(dict, value);
    }
    else if (strcmp(command, "bitcode") == 0){
        // check option value from 0 to 255
        int value = get_value(option);
        // get the bitcode of the given value
        get_bitcode(dict, value);
    }
    else if (strcmp(command, "compress") == 0) {
        unsigned char *compression_message = encode_msg(dict, buffer, filelen);
        write_msg_to_file(dict, compression_message, option);
        free(compression_message);
    }
    else if (strcmp(command, "dictionary") == 0) {
        unsigned char *compression_message = create_dictionary(dict);
        write_msg_to_file(dict, compression_message, option);
        free(compression_message);
    }
    free(dict->nodes);
    free(dict);
    free(buffer);
    return 0;
}


