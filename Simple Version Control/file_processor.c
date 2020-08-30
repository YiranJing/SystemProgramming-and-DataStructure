#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <ftw.h>
#include "svc.h"
#include "rmtree.h"
#include "file_processor.h"

/*
    Free histroical commits in the given branch
*/
void clean_branch_commit(struct svc_branch_list *branches) {
    // free all histroical commits
    for (int i = 0; i < branches->n_commit; i++) {

        struct commit *commit = &branches->commit_list[i];

        free_ptr(commit->commit_id);
        free_ptr(commit->message);

        // free temp folers
        rmtree(commit->dir_name);
        free(commit->dir_name);

        if (commit->n_tracked_files > 0) {
            free_ptr(commit->tracked_files);
        }

        if (commit->n_added_files > 0) {
            free_ptr(commit->added_files);
        }

        if (commit->n_removed_files > 0) {
            free_ptr(commit->removed_files);
        }

        if (commit->n_modified_files > 0) {
            free_ptr(commit->modified_files);
            free(commit->modified_old_hash);
        }

        if (commit->n_parent_commit > 0) {
            free_ptr(commit->parent_commit);
        }
    }
}


/*
  Free pointer if it is not null
  and set to NULL after free it
*/
void free_ptr(void * ptr) {
  if (ptr != NULL) {
    free(ptr);
    ptr = NULL;
  }
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the majority of the following function has been copied from
 * includehelp website with only minor changes and it is not my own work.
 *
 * https://www.includehelp.com/c/convert-ascii-string-to-byte-array-in-c.aspx
 */
void string2ByteArray(char* input, BYTE* output) {
    int loop = 0;
    int i = 0;
    while(input[loop] != '\0') {
        output[i] = (int)input[loop];
        i++;
        loop++;
    }
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the majority of the following function has been copied from
 * wiki.sei.cmu.edu website with only minor changes and it is not my own work.
 *
 * https://wiki.sei.cmu.edu/confluence/display/c/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+regular+file
 */
off_t calculate_size_of_file_content(char *file_path) {
    // Calculate the size of the given file
    struct stat st;
    stat(file_path, &st);
    off_t size = st.st_size;
    return size;
}


/*
    Calculate the sum of elements in an array of size n
*/
int sum_array(BYTE arr[], int n) {
    int sum = 0; // initialize sum

    // Iterate through all elements
    // and add them to sum
    for (int i = 0; i < n; i++) {
        sum += (int) arr[i];
    }
    return sum;
}


/*
    Calculate commit id based on byte array of changed file name
*/
int sum_id_file_name(int id, BYTE arr[], int n) {
        for (int i = 0; i < n; i++){
            int b = (int) arr[i];
            id = (id * (b % 37)) % 15485863 + 1;
            //printf("line 114 id %d \n", id);
        }
    return id;
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the majority of the following function has been copied from
 * stackoverflow website with only minor changes and it is not my own work.
 *
 * https://stackoverflow.com/questions/23618316/undefined-reference-to-strlwr
 */
char *strlwr(char *str) {
  unsigned char *p = (unsigned char *)str;
  // Convert string to lower case
  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }
  return str;
}


/*
    Sort intput 'str' (array of strings) in increasing alphabetical order
*/
void sort_array_of_string(char **str, size_t count) {
    size_t i,j;
    char *temp;
    char *lower_str_i = NULL;
    char * lower_str_j = NULL;

	/* USYD CODE CITATION ACKNOWLEDGEMENT
 	* I declare that the majority of the following function has been copied from
 	* beginnersbook website with only minor changes and it is not my own work.
 	*
 	* https://beginnersbook.com/2015/02/c-program-to-sort-set-of-strings-in-alphabetical-order/
 	*/
    for(i=0; i< (count-1); i++) {
       for(j= i+1; j < count; j++) {
          lower_str_i = strdup(str[i]);
          lower_str_j = strdup(str[j]);

          lower_str_i = strlwr(lower_str_i);
          lower_str_j = strlwr(lower_str_j);

          // compare the lower case of each char
          if(strcmp(lower_str_i, lower_str_j )> 0) {
             temp = *(str + i);
             *(str + i) =  *(str + j);
             *(str + j) = temp;
          }
          free_ptr(lower_str_i);
          free_ptr(lower_str_j);
       }
    }
}


/*
	Sort file names in increasing alphabetical order of the file name
*/
char **sort_changed_files(struct commit *new_commit, size_t count) {

    char **raw_file_name = calloc(count, sizeof(char *));
    int i = 0;
    for(size_t j = 0; j < new_commit->n_added_files; j ++) {
        raw_file_name[i] = new_commit->added_files[j].file_path;
        i++;
    }
    for(size_t j = 0; j < new_commit->n_removed_files; j ++) {
        raw_file_name[i] = new_commit->removed_files[j].file_path;
        i++;
    }
    for(size_t j = 0; j < new_commit->n_modified_files; j ++) {
        raw_file_name[i] = new_commit->modified_files[j].file_path;
        i++;
    }

    if (count != i) {
       puts("error!");
    }

    sort_array_of_string(raw_file_name, count);

    return raw_file_name;
}


/*
  	Get the file name from the given file path
*/
char *get_file_name(char *src_path) {
  char *file_name;
  const char s[CHAR_LENGTH] = "/";
  char *token;

  /* get the first token */
  token = strtok(src_path, s);
  file_name = token;

  /* walk through other tokens */
  while(token != NULL ) {
      file_name = token;
      token = strtok(NULL, s);
   }
  return file_name;
}


/*
  	Helper function of copy_file
*/
void copy_file_content(char *dest_path, char *src_path) {
   FILE *source, *target;
   char ch;

   /* USYD CODE CITATION ACKNOWLEDGEMENT
 	* I declare that the majority of the following function has been copied from
 	* forgetcode website with only minor changes and it is not my own work.
 	*
 	* https://forgetcode.com/c/577-copy-one-file-to-another-file
 	*/
   source = fopen(src_path, "r");

   if( source == NULL )
   {
      printf("Fail to read file [%s]\n", src_path);
      exit(EXIT_FAILURE);
   }

   target = fopen(dest_path, "w");

   if( target == NULL ) // need to create sub_dir
   {
      perror("\ncopy temp file failed");
      fclose(source);
      puts(dest_path);
      exit(EXIT_FAILURE);
   }

   while( (ch = fgetc(source) ) != EOF )
      fputc(ch, target);

   fclose(source);
   fclose(target);
}


/* USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the majority of the following function has been copied from
 * forgetcode website with only minor changes and it is not my own work.
 *
 * https://forgetcode.com/c/577-copy-one-file-to-another-file
 */
void copy_file(char *dir_name, char *src_path) {

   // Copy the file content from src_path to dest_path
   char *temp_path = strdup(src_path);
   char *src_file_name = get_file_name(temp_path);

   // create dest_path
   char* dest_path = calloc(1, BUF_LEN);
   strcpy(dest_path, dir_name); // copy content
   strcat(dest_path, "/");
   strcat(dest_path, src_file_name);

   copy_file_content(dest_path, src_path);
   free(dest_path);
   free(temp_path);
}
