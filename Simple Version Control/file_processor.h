#ifndef file_processor_h
#define file_processor_h


void clean_branch_commit(struct svc_branch_list *branches);

void free_ptr(void * ptr);

void string2ByteArray(char * input, BYTE* output);

off_t calculate_size_of_file_content(char * file_path);

int sum_array(BYTE arr[], int n);

int sum_id_file_name(int id, BYTE arr[], int n);

char *strlwr(char *str);

void sort_array_of_string(char **str, size_t count);

char **sort_changed_files(struct commit *new_commit, size_t count);

char *get_file_name(char *src_path);

void copy_file_content(char *dest_path, char *src_path);

void copy_file(char *dir_name, char *src_path);

#endif
