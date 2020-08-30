#ifndef HUFFMAN_H
#define HUFFMAN_H

#define DICT_SIZE (256)
#define BYTE_SIZE (4)
#define MAX_BUFF (1024)

// Usage example:
// clang -g -Wall -Werror -std=gnu11 huffman.c -o huffman -lpthread -fsanitize=address
// ./huffman sample-input.txt frequency 98
// ./huffman sample-input.txt bitcode 98
// ./huffman sample-input.txt compress compress_output
// ./huffman sample-input.txt dictionary dictionary_output

// Huffman tree node
struct tree_node{
    uint8_t key; /* 0-255 */
    uint64_t freq;
    struct tree_node *left;
    struct tree_node *right;
};
  

struct huff_tree {
    uint64_t size;
    struct tree_node *nodes[DICT_SIZE];
};

struct compress_dict {
    struct binary_tree *tree;
    uint8_t dict_bit_array[DICT_SIZE * 4]; /* bit array for compression infor */
    int seg_index[DICT_SIZE + 1]; /* start index of 256 segments and padding */
    uint64_t freq[DICT_SIZE]; /* the frequency of 256 segments based on the given target file*/
    struct freq_node* nodes; /* node list for input symbols in the trget file */
    struct tree_node* root; /* huffman tree */
    int compress_len;
};

struct freq_node{
    uint8_t key; // unique value for each symbol (0-255)
    uint64_t value; /* the frequency of the key */
    uint8_t bit_array[DICT_SIZE]; /* bit code of the node, saved as bitaray */
    uint8_t length; /* length of bit array, maximum is 256 */
    uint8_t n_padding; /* the number of paddings in the bit array */
};


void set_bit(uint8_t* array, int index);

void clear_bit(uint8_t *array, int index);

uint8_t get_bit(uint8_t* array, int index);

struct huff_tree* tree_init(uint8_t *key, uint64_t *freq, int size);

void update_tree(struct huff_tree* huff_tree);

void update_tree_helper(struct huff_tree* huff_tree, uint64_t left,
                        uint64_t right, uint64_t i);

struct tree_node* get_mini_freq_node(struct huff_tree* huff_tree);

void insert_huff_tree(struct huff_tree* huff_tree, struct tree_node *right,
                      struct tree_node *left, uint64_t sum);

struct tree_node* create_huffman_tree(uint8_t *key, uint64_t *freq, int size);

int compare(const void * a, const void * b);

uint8_t *read_file_byte(char *file, uint64_t *filelen);

uint64_t *calculate_frequency(uint8_t *arr, uint64_t n, uint64_t *freq);

void generate_node_list(struct compress_dict *dict);

void get_frequency(struct compress_dict *dict, int index);

int get_value(char *option);

void save_bit_array(int *arr, int n, uint8_t *bit_array);

void save_codes(struct tree_node* root, int* arr, int top,
                struct compress_dict *dict);

void tree_destroy(struct tree_node* root);

void calculate_bitcode(struct compress_dict *dict);

void get_bitcode(struct compress_dict *dict, int index);

unsigned char *encode_msg(struct compress_dict *dict, uint8_t *buffer,
                          uint64_t filelen);

unsigned char *create_dictionary(struct compress_dict *dict);

void write_msg_to_file(struct compress_dict *dict,
                       unsigned char *compression_message, char *option);


#endif



