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

#define  _POSIX_C_SOURCE 200809L
#define  _XOPEN_SOURCE 500L


/*
    Perform preparation steps by create "master" branch, which is called once '
    in the begining
    The returned "helper" pointing to the area of "master" branch.
*/
void *svc_init(void) {
    struct svc_branch_list * helper = calloc(1, sizeof(struct svc_branch_list));
    helper->branch_list = calloc(1, sizeof(struct svc_branch));
    helper->branch_list[0].branch_name = strdup("master");
    helper->branch_list[0].valid = 1;
    helper->n_branches = 1; // initially only master
    helper->current_position = 0; //the index of master branch
    helper->n_commit = 0;
    return (void *)helper;
}


/*
    Get the current branch based on branch list
*/
struct svc_branch * get_current_branch_ptr(void *helper) {
    struct svc_branch_list *branches = (struct svc_branch_list *)helper;
    int current_position = branches->current_position;
    struct svc_branch *branch_ptr = &branches->branch_list[current_position];
    return branch_ptr;
}


/*
    Helper function for svc_merge
    Get ptr of the merged branch based on the branch name
*/
struct svc_branch * get_merged_branch_ptr(void *helper, char *branch_name) {
  struct svc_branch_list *branches = (struct svc_branch_list *)helper;
  for (int i = 0; i<branches->n_branches; i++) {
    if (strcmp(branches->branch_list[i].branch_name, branch_name) == 0) {
      return &branches->branch_list[i];
    }
  }
  return NULL;
}


/*
    Free all dynamically allocated memory in the end
*/
void cleanup(void *helper) {

    // get all historical branches
    struct svc_branch_list *branches = (struct svc_branch_list *)helper;

    // clean each branch
    for (int k = 0; k< branches->n_branches; k++) {

       struct svc_branch *branch_ptr =  &branches->branch_list[k];

       for (int i = 0; i < branch_ptr->n_files; i++) {
              free_ptr(branch_ptr->files[i].file_path);
        }
       free_ptr(branch_ptr->files);
       free_ptr(branch_ptr->branch_name);
       free_ptr(branch_ptr->commit_histroy);
    }

    // clean commit
    clean_branch_commit(branches);
    free_ptr(branches->commit_list);
    free_ptr(branches->branch_list);
    free_ptr(branches);
}


/*
   Calculate and return the hash value of the file
   This function works for both tracked and untrakced files
*/
int hash_file(void *helper, char *file_path) {

    // Return -1 if file path is NULL
    if (file_path == NULL) {
        return -1;
    }

    FILE * f = fopen(file_path, "r"); // read only
    // check if file exist at the given path
    if (f == NULL) { // unable to open file
        int errnum = errno;
        if (errnum==2) { // No such file or directory
            return -2;
        }
    }

    // convert ASCII char[] to BYTE array
    BYTE *arr = malloc(sizeof(int)*strlen(file_path));

    string2ByteArray(file_path, arr); //converting string to BYTE[]

    // hash calculation based on file name
    int hash = 0;
    int sum = sum_array(arr, strlen(file_path));
    hash = (hash + sum)  % 1000;
    free(arr);

    // hash calculation based on file contents
    unsigned char buf;
    while(fscanf(f, "%c", &buf) != EOF) {
      hash = (hash + (unsigned char)buf) % 2000000000;
    }
    fclose(f);
    return hash;
}


/*
    Search and confirm the given file is added, removed or modified
    in the given commit

    Return 1: if added; Return 2: if removed; Return 3: if modified.
    Return -1 if cannot match;
*/
int match_file_name(char *file_name, struct commit *new_commit) {

    for(size_t j = 0; j < new_commit->n_added_files; j ++) {
        if (strcmp(new_commit->added_files[j].file_path, file_name) == 0)
            return 1;
    }
    for(size_t j = 0; j < new_commit->n_removed_files; j ++) {
        if (strcmp(new_commit->removed_files[j].file_path, file_name) == 0)
            return 2;
    }
    for(size_t j = 0; j < new_commit->n_modified_files; j ++) {
        if (strcmp(new_commit->modified_files[j].file_path, file_name) == 0)
            return 3;
    }
    return -1; // fail to match
}


/*
    Calculate and return the commit ID based on the example given
*/
char *calculate_commit_id(char *message, struct commit *new_commit,
                          char *commit_id) {
    int id = 0;
    // convert commit.message to BYTE array
    BYTE *arr = malloc(sizeof(int)*strlen(message));
    string2ByteArray(message, arr);
    int sum = sum_array(arr, strlen(message));
    id = (id + sum) % 1000;

    // sort files in commit's changes(add, rm, or modified)
    // in increasing alphabetical order
    size_t count = new_commit->n_added_files + new_commit->n_removed_files
                                             + new_commit->n_modified_files;
    // sorted strin g array of file_names
    char **sorted_files = sort_changed_files(new_commit, count);
    // calculate id based on the each changed file (sorted)
    for (size_t j = 0; j < count; j++) {

        char *file_name = sorted_files[j];
        // increase id based on the type of changed file
        int match = match_file_name(file_name, new_commit);
        if (match == 1) { // match = 1 means the file is newly added
            id = id + 376591;
        }
        else if (match == 2) { // match = 1 means the file is removed
            id = id + 85973;
        }
        else { // match = 3 means the file is modified
            id = id + 9573681;
        }
        // Incrase id based on unsighed byte of file name
        arr = (BYTE *) realloc(arr, sizeof(int)*strlen(file_name));
        string2ByteArray(file_name, arr); //converting string to BYTE[]
        id = sum_id_file_name(id, arr, strlen(file_name));
    }
    free(sorted_files);
    free(arr);
    // Convert id as 6 digit hexadecimal string
    sprintf(commit_id, "%06x", id);
    return commit_id;
}


/*
    Add the removed file to the "removed_files" list of current commit
*/
void add_removed_file_to_commit(void *helper, struct svc_file *file,
                                struct commit *new_commit,
                                struct svc_branch *branch_ptr,
                                int *valid_commit) {
  file->removed = 0;
  file->added = 0;
  file->staged = 0;
  file->modified = 0;

  // check and update file's hash code
  file->hash_code = hash_file(helper, file->file_path);
  *valid_commit = 1;
  new_commit->n_removed_files++;
  new_commit->removed_files = realloc(new_commit->removed_files,
                          sizeof(struct svc_file)*new_commit->n_removed_files);
  // add removed file
  new_commit->removed_files[new_commit->n_removed_files-1] = *file;
  new_commit->n_parent_commit = 1; // one parent commit of current branch
  if (branch_ptr->n_commit > 0) {
      struct commit * cm_history = branch_ptr->commit_histroy;
      new_commit->parent_commit = &cm_history[branch_ptr->n_commit - 1];
  }
  new_commit->parent_commit = NULL; // the first commit
}


/*
    Count currenlt tracked files in the given commit
    The currently tracked files are in:
    added_files, modified_files and tracked_files
*/
int count_trancked_f(struct commit *new_commit) {
  int count = new_commit->n_added_files + new_commit->n_modified_files;
  count = count + new_commit->n_tracked_files;
  return count;
}


/*
    Store all tracked files of the given commit on current branch to
    "temp directory folder"

    The currently tracked files are in:
    added_files, modified_files and tracked_files
*/
void store_tracked_f(void *helper, char *dir_name, char *commit_id,
                    struct commit *commit) {

  int count = count_trancked_f(commit);

  if (count == 0) {
    // no tracked file need to be copied
    return;
  }

  for (int i = 0; i< commit->n_tracked_files; i++) {

    char *src_path = commit->tracked_files[i].file_path;
    copy_file(dir_name, src_path);
  }

  for (int i = 0; i< commit->n_added_files; i++) {

    char *src_path = commit->added_files[i].file_path;
    copy_file(dir_name, src_path);
  }

  for (int i = 0; i< commit->n_modified_files; i++) {

    char *src_path = commit->modified_files[i].file_path;
    copy_file(dir_name, src_path);

  }
}


/*
    Check and update commit if the given file is mannully deleted
    Return -1 if the given file is manully deleted
*/
int check_manually_removed_file(void *helper, struct svc_file *file,
                                struct commit *new_commit, int *valid_commit) {

  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

  FILE * f = fopen(file->file_path, "r");
  if (f == NULL) { // cannot open file successfully means file is removed
    if (file->staged == 0 && file->added == 1){
      // do nothing for the files in the staging phrase
      add_removed_file_to_commit(helper, file, new_commit, branch_ptr,
                                  valid_commit);
    }
    else{
      // remove files in the staging phrase
      file->staged = 0;
      file->added = 0;
      file->removed = 0;
    }
    return -1;
  }
  return 0;
}


/*
    Helper function for svc_commit:
    Search and update overall_commit_list
*/
void update_overall_commit_list(void *helper, struct commit * new_commit,
                                struct commit * parent_commit) {

  // insert new_commit the overall commit_list
  struct svc_branch_list *branches = (struct svc_branch_list *)helper;
  //struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

  branches->n_commit++;
  branches->commit_list = realloc(branches ->commit_list,
                                  sizeof(struct commit) * branches->n_commit);
  branches->commit_list[branches->n_commit - 1] = *new_commit;
}



/*
    Helper function for dectected_changes_in_f:
    update the modefied files for current commit
*/
void update_modified_files(void *helper, struct commit *new_commit,
                      struct svc_file *file, int *valid_commit) {

  int hash2 = hash_file(helper, file->file_path);

  // detected new changed the tracked files by checking hash_code
  if(hash2 != file->hash_code) {
      *valid_commit = 1;
      new_commit->n_modified_files++;
      int n_mod = new_commit->n_modified_files;
      new_commit->modified_files = realloc(new_commit->modified_files,
                                  sizeof(struct svc_file)*n_mod);
      new_commit->modified_old_hash = realloc(
                     new_commit->modified_old_hash, sizeof(int)*n_mod);
      new_commit->modified_old_hash[n_mod-1] = file->hash_code;
      file->hash_code = hash2; // update the hash code for changed file
      new_commit->modified_files[n_mod-1] = *file; // add new file
  }
  else {
      // collected unchanged tracked files
      new_commit->n_tracked_files++;
      new_commit->tracked_files = realloc(new_commit->tracked_files,
                  sizeof(struct svc_file)*new_commit->n_tracked_files);
      new_commit->tracked_files[new_commit->n_tracked_files-1] = *file;
  }
}


/*
    Helper function for svc_commit:
    Detect if any changes in the file: add, rm, or modified
*/
int dectected_changes_in_f(void *helper, struct svc_branch *branch_ptr,
                              struct commit *new_commit) {

  int valid_commit = 0; // invalid commit if no change can be found

  // Detected new added file (i.e. files in the staging phrase)
  for (size_t i = 0; i<branch_ptr->n_files; i++){
      struct svc_file *file = &branch_ptr->files[i];
      // Remove scenario 1: manually delete files.
      // we shall check if currently tracked files can be opened successfully
      int find = check_manually_removed_file(helper, file,
                                                new_commit, &valid_commit);
      if (find <0){ continue; }
      // Detect new added files
      if (file->added == 1 && file->staged == 1) {
          valid_commit = 1;
          //check and update the hash code for the new added file
          int hash2 = hash_file(helper, file->file_path);
          if(hash2 != file->hash_code) { file->hash_code = hash2; }
          file->staged=0;
          new_commit->n_added_files++;
          new_commit->added_files = realloc(
                            new_commit->added_files,
                            sizeof(struct svc_file)*new_commit->n_added_files);
          // add new file
          new_commit->added_files[new_commit->n_added_files-1] = *file;
          continue;
      }
      // check tracked files
      else if (file->added == 1 && file->staged == 0 && file->removed ==0) {
         // update modified files for the new commit
         update_modified_files(helper, new_commit, file, &valid_commit);
      }
      // Remove scenario 2: rm files by svc_rm function
      else if (file->added == 0 && file->removed ==1) {
        add_removed_file_to_commit(helper, file, new_commit, branch_ptr,
                                  &valid_commit);
      }
  }
  return valid_commit;
}


/*
    Helper function for svc_commit
    Create temo directory and store all tracked file inside for the new commit
*/
void store_in_temp_dir(void *helper, char *commit_id,
                       struct commit *new_commit) {

    /* Create the temporary directory */
    char template[TEMP_PATH] = "/tmp/XXXXXX";
    char *dir_name = mkdtemp(template); //需要free
    if(dir_name == NULL) {
          perror("mkdtemp failed: ");
          return;
    }

    // store tracked file's content
    store_tracked_f(helper, dir_name, commit_id, new_commit);
    new_commit->dir_name = strdup(dir_name);
}


/*
    Helper function for svc_commit
    Add parent commit information to the new commit
*/
void add_parent(void *helper, struct commit *new_commit, char *message,
          struct svc_branch *branch_ptr) {

  // get the lastest commit in current branch
  struct commit * parent_commit;

  // the very initial commit doesnot have parent commit.
  if (branch_ptr->n_commit >= 1) {

    // get the lastest commit in current branch
    parent_commit = &branch_ptr->commit_histroy[branch_ptr->n_commit - 1];

    // update the parent commit information
    new_commit->n_parent_commit = 1; // can only has 1 parent without merge
    new_commit->parent_commit = realloc(new_commit->parent_commit,
                                                  sizeof(struct commit) * 1);
    new_commit->parent_commit[0] = *parent_commit;
  }

  // Update commit history in current branch
  branch_ptr->n_commit++;
  branch_ptr->commit_histroy = realloc(branch_ptr->commit_histroy,
                                 sizeof(struct commit) * branch_ptr->n_commit);

  // get new allocation of parent_commit
  if (branch_ptr->n_commit>=2) {
      parent_commit = &branch_ptr->commit_histroy[branch_ptr->n_commit - 2];
  }
  else {
      parent_commit = &branch_ptr->commit_histroy[branch_ptr->n_commit - 1];
  }

  branch_ptr->commit_histroy[branch_ptr->n_commit - 1] = *new_commit;

  // update the overall commit_list
  update_overall_commit_list(helper, new_commit, parent_commit);

}


/*
    Create a commit with the message given,
    and all changes to file being tracked
*/
char *svc_commit(void *helper, char *message) {

    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

    if (message == NULL || branch_ptr->n_files == 0) {
        return NULL;
    }
    // create new commit
    struct commit *new_commit = calloc(1, sizeof(struct commit));

    /* Detect if any changes in the file: add, rm, or modified */
    int valid_commit = dectected_changes_in_f(helper, branch_ptr, new_commit);

    if (valid_commit == 0) {
      free(new_commit->tracked_files);
      free(new_commit);
      return NULL;
    }

    if (valid_commit == 1){
        // calculate the commit ID
        char *commit_id = calloc(1, sizeof(char *));
        commit_id = calculate_commit_id(message, new_commit, commit_id);

        // update new commit's content
        new_commit->message = strdup(message); // store commit message
        new_commit->commit_id = strdup(commit_id); //store commit_id

        // Create temo directory and store tracked file inside the new commit
        store_in_temp_dir(helper, commit_id, new_commit);

        // add parent commit information to the new commit
        add_parent(helper, new_commit, message, branch_ptr);

        free(commit_id);
        free(new_commit);
        return branch_ptr->commit_histroy[branch_ptr->n_commit - 1].commit_id;
    }

    // if no change since the last commit
    free(new_commit);
    return NULL;
}

/*
    Helper function of svc_add
    add new file to the current branch
    Return hash value of the new file
*/
int add_file_to_brnach(void *helper, char *file_name,
                        struct svc_branch *branch_ptr) {

  // calculate the hash value
  int hash = hash_file(helper, file_name);

  branch_ptr->n_files++; // add new tracked file
  branch_ptr->files = realloc(branch_ptr->files,
                                 sizeof(struct svc_file)*branch_ptr->n_files);
  branch_ptr->files[branch_ptr->n_files-1].hash_code = hash;
  branch_ptr->files[branch_ptr->n_files-1].file_path = strdup(file_name);
  branch_ptr->files[branch_ptr->n_files-1].added = 1;
  // initially in the staged phrase
  branch_ptr->files[branch_ptr->n_files-1].staged = 1;
  // initially no removed
  branch_ptr->files[branch_ptr->n_files-1].removed = 0;
  // initially no modified
  branch_ptr->files[branch_ptr->n_files-1].modified = 0;

  return hash;
}


/*
    Add file to the version control
*/
int svc_add(void *helper, char *file_name) {

    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

    if (file_name == NULL) // check invalid input
        return -1;

    // check if the file exists
    FILE * f = fopen(file_name, "r"); // read only
    // check if file exist at the given path
    if (f == NULL) { // unable to open file
       return -3;
    }
    fclose(f);

    // check if added to svc of this branch before
    for (size_t i = 0; i<branch_ptr->n_files; i++) {

        struct svc_file *file = &branch_ptr->files[i];
        if (strcmp(file->file_path, file_name) == 0 && file->added == 1
                                                    && file->removed ==0) {
            return -2; // the file is already been tracked in this branch
        }
        // the file is removed and re-added
        if (strcmp(file->file_path, file_name) == 0 && file->added == 0) {
            int hash = hash_file(helper, file_name);
            file->hash_code = hash;
            file->added = 1;
            file->staged = 1;
            file->removed = 0;
            file->modified = 0;
            return hash;
        }
    }
    // otherwise, add new file to the current branch
    int hash = add_file_to_brnach(helper, file_name, branch_ptr);

    return hash;
}


/*
    Get commit from any branch
    Return a pointer to the area of memory stored this commit
    Return Null if the given id is not exist or commit_id is NULL
*/
void *get_commit(void *helper, char *commit_id) {

    if (commit_id == NULL) { // invalid input
        return NULL;
    }

    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);
    // check given commit is valid
    for (int j = 0; j < branch_ptr->n_commit; j++) {

        // check commit_id is created before
        if (strcmp(branch_ptr->commit_histroy[j].commit_id, commit_id) == 0) {
            void *commit = &branch_ptr->commit_histroy[j];
            return commit;
        }
    }
    return NULL; // cannot find id
}


/*
    Return a dynamically allocated array of parent_commits
*/
char **get_prev_commits(void *helper, void *commit, int *n_prev) {

    if (commit == NULL) { // invalid input
        *n_prev = 0;
        return NULL;
    }

    struct commit *current_commit = (struct commit *) commit;

    // return NULL if it is the very first commit
    if (current_commit->n_parent_commit == 0) {
        *n_prev = 0;
        return NULL;
    }

    *n_prev = current_commit->n_parent_commit;
    char ** parent_commit = calloc(*n_prev, sizeof(char *));

    for (int i = 0; i < *n_prev; i++) {
        parent_commit[i] = current_commit->parent_commit[i].commit_id;
    }
    return parent_commit;
}


/*
    Helper functon of print_commit
    Print the commit centent, based on the given valid pointer of stored commit
*/
void print_layout_commit(struct svc_branch *branch_ptr, struct commit *commit) {
    printf("%s [%s]: %s\n", commit->commit_id, branch_ptr->branch_name,
           commit->message);

    // print added files
    for (size_t i = 0; i<commit->n_added_files; i++){
        printf("    + %s\n", commit->added_files[i].file_path);
    }

    for (size_t i = 0; i<commit->n_removed_files; i++){
        printf("    - %s\n", commit->removed_files[i].file_path);
    }

    for (size_t i = 0; i<commit->n_modified_files; i++){
        printf("    / %s", commit->modified_files[i].file_path);
        printf(" [%d --> %d]\n", commit->modified_old_hash[i],
               commit->modified_files[i].hash_code);
    }
    int count = count_trancked_f(commit);
    printf("\n    Tracked files (%d):\n", count);

    // print currently tracked files
    for (size_t i = 0; i<commit->n_tracked_files; i++){
        struct svc_file file = commit->tracked_files[i];
        printf("    [%10d] %s\n", file.hash_code, file.file_path);
    }

    for (size_t i = 0; i<commit->n_added_files; i++){
        struct svc_file file = commit->added_files[i];
        printf("    [%10d] %s\n", file.hash_code, file.file_path);
    }

    for (size_t i = 0; i<commit->n_modified_files; i++){
        struct svc_file file = commit->modified_files[i];
        printf("    [%10d] %s\n", file.hash_code, file.file_path);
    }
}


/*
    Print the details of the given commit
*/
void print_commit(void *helper, char *commit_id) {
    //printf("call print_commit\n");
    if (commit_id == NULL) { // invalid input
        puts("Invalid commit id");
        return;
    }
    struct svc_branch_list *branches = (struct svc_branch_list *)helper;

    // check the given commit_id exits
    for (int i = 0; i<branches->n_branches; i++) {
      struct svc_branch *branch_ptr = &branches->branch_list[i];
      for (int j = 0; j < branch_ptr->n_commit; j++) {
        // check commit_id is created before
        if (strcmp(branch_ptr->commit_histroy[j].commit_id, commit_id) == 0) {
          print_layout_commit(branch_ptr, &branch_ptr->commit_histroy[j]);
          return;
      }
    }
  }
    puts("Invalid commit id");
}


/*
    Check if given digit is between 0 and 9
    Return 0 if the given digit is between 0 and 9, otherwise return -1
*/
int check_valid_digit(char ch) {
    int d = (int) ch;
    if (d >=0 && d <=9){
        return 0;
    }
    else{
        return -1; // find invalid digit
    }
}


/*
    Check if the given branch name is valid
    valid branch names are restricted to those that
    contain only a-z, A-Z, 0-9, _, /, -
    Return 0 if valid, otherwise return -1
*/
int check_valid_branch_name(char *branch_name) {
    int i = 0;
    while (1) {
        char ch = branch_name[i];
        // check it is digit from 0-9
        if (isdigit(ch)){
            if (check_valid_digit(ch)){
                i++;
                continue;
            }
            else{
                return -1; // invalid digit
            }
        }
        // check alphanumeric characters, underscores, slashes and dashes
        if (isalnum(ch) || (ch == '_')|| (ch == '/') || (ch == '-')) {
            //printf("ch: %c \n", ch);
            i++;
            continue;
        }
        if (i == strlen(branch_name)){ // finish check all string
            break;
        }
        return -1; // find invalid char
    }
    return 0;
}


/*
    Check if exits uncommitted changes in the given branch
    Return -1 if find uncommitted changes, otherwise return 0
*/
int check_uncommitted_changes(struct svc_branch *branch_ptr) {
  //("line 1175 check uncommitted changes!\n");
    for (size_t j = 0; j < branch_ptr->n_files; j++) {
        struct svc_file file = branch_ptr->files[j];
        if (file.staged == 1) {
          //puts("line 1179 find uncommitted changes!\n");
            return -1; // find uncommitted changes
        }
    }
    return 0;
}


/*
    Check if the tracked file has been modified
*/
int check_changes_tracked_file(void *helper, struct svc_branch *branch_ptr) {
  //puts("line 1175 check uncommitted changes!\n");
    for (size_t j = 0; j < branch_ptr->n_files; j++) {
        struct svc_file file = branch_ptr->files[j];
        int hash = svc_add(helper, file.file_path);
        if (hash!= -2 && hash != file.hash_code && file.added == 1) {
            return -1; // find uncommitted changes
        }
    }
    return 0;
}


/*
    Check if branch_name is already exist
    Return 0 if already exists
    Otherwise return -1
*/
int check_branch_exist(void *helper, char *branch_name) {
  // check if branch_name is already exist
  struct svc_branch_list *branches = (struct svc_branch_list *)helper;
  for (int i = 0; i< branches->n_branches; i++) {
      char *old_bra_name = branches->branch_list[i].branch_name;
      if (strcmp(old_bra_name, branch_name) == 0){ // branch_name already exists
          return 0;
      }
  }
  return -1;
}


/*
    Helper function of svc_branch:
    Create new branch based on the given valid branch name
*/
void create_new_branch(void *helper, char *branch_name) {

  struct svc_branch_list *branches = (struct svc_branch_list *)helper;
  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

  // create a new branch, which is the copy of current active branch
  branches->n_branches++;
  branches->branch_list = realloc(branches->branch_list,
                               sizeof(struct svc_branch) * branches->n_branches);
  // update pointer address after realloc!
  branch_ptr = get_current_branch_ptr(helper);
  branches->branch_list[branches->n_branches - 1].valid = 1;
  char *name = branch_name;
  branches->branch_list[branches->n_branches - 1].branch_name = strdup(name);

  // copy all files from current active branch
  size_t n_copy_files = branch_ptr->n_files;
  branches->branch_list[branches->n_branches - 1].n_files = n_copy_files;

  // the file tracked in each branch must be independent
  branches->branch_list[branches->n_branches - 1].files = calloc(n_copy_files,
                                                       sizeof(struct svc_file));
  memmove(branches->branch_list[branches->n_branches - 1].files,
         branch_ptr->files,
         sizeof(struct svc_file) * n_copy_files);
  // deepcopy file name
  for (size_t k = 0; k < n_copy_files; k++){
      char * file_name = branch_ptr->files[k].file_path;
      int n_bran = branches->n_branches;
      struct svc_file * bran_files = branches->branch_list[n_bran - 1].files;
      bran_files[k].file_path = strdup(file_name);
  }

  // the first commit in the new branch is current commit
  branches->branch_list[branches->n_branches - 1].n_commit = 1;
  branches->branch_list[branches->n_branches - 1].commit_histroy = calloc(
                                   branch_ptr->n_commit, sizeof(struct commit));
  int n_com = branch_ptr->n_commit;
  struct commit *last_commit = &branch_ptr->commit_histroy[n_com - 1];
  int n_bran = branches->n_branches;
  struct commit *bran_c_hist = branches->branch_list[n_bran - 1].commit_histroy;
  bran_c_hist[0] = *last_commit;
  bran_c_hist[0].dir_name = last_commit->dir_name;
}


/*
    Create a new branch with the given name
    Return -1: if invalid branch_name or NULL
    Return -2: if the branch_name is already exist
    Return -3: if there are uncommitted changes
*/
int svc_branch(void *helper, char *branch_name) {

    // check if valid branch_name
    if (branch_name == NULL)
        return -1;
    int check = check_valid_branch_name(branch_name);

    if (check < 0) {
        return -1;
    }

    // check if branch_name is already exist
    check = check_branch_exist(helper, branch_name);
    if (check == 0) {
      return -2;
    }

    // check if there are uncommitted changes on current branch
    // (i.e. some files in staging phase)
    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

    check = check_uncommitted_changes(branch_ptr);
    if (check <0) {
        return -3;
    }

    // Create new branch based on the given valid branch name
    create_new_branch(helper, branch_name);

    return 0; // successful return
}


/*
    Make this branch the active one
    Return -1: if NULL or no such branch exists
    Return -2: if there are uncommitted changes
    Return 0: succefully make it the active branch
*/
int svc_checkout(void *helper, char *branch_name) {

    if (branch_name == NULL)
        return -1;

    // check if branch_name is already exist
    int check = 0;
    struct svc_branch_list *branches = (struct svc_branch_list *)helper;
    for (int i = 0; i< branches->n_branches; i++) {
        char *old_bra_name = branches->branch_list[i].branch_name;
        if (strcmp(old_bra_name, branch_name) == 0){
            check = 1; // branch_name already existed
        }
    }
    if (check != 1)
        return -1;

    // check if there are uncommitted changes on current branch
    // (i.e. some files in staging phase)
    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);
    check = check_uncommitted_changes(branch_ptr);
    if (check <0) {
        return -2;
    }

    // make it the active branch
    for (int i = 0; i< branches->n_branches; i++) {
        struct svc_branch *branch = &branches->branch_list[i];
        if (strcmp(branch->branch_name, branch_name) == 0) { // find branch
            branches->current_position = i; // mark current branch as active

            // get the new active branch
            branch_ptr = get_current_branch_ptr(helper);
            struct commit * bran_com_hist = branch->commit_histroy;
            struct commit *last_commit = &bran_com_hist[branch->n_commit - 1];

            // restore the files on the new active branch
            undo_file_changes(helper, last_commit);
            return 0;
        }
    }
    return 0;
}


/*
    Print all the branches in the order they were created
    Return a dunamically allocated array of the branch names in the same order
*/
char **list_branches(void *helper, int *n_branches) {

    if (n_branches == NULL)
        return NULL;

    struct svc_branch_list *branches = (struct svc_branch_list *)helper;
    *n_branches = branches->n_branches;
    char ** branch_names = calloc(*n_branches, sizeof(char *));

    // iteration over all created branches
    for (int i = 0; i < *n_branches; i++) {
        branch_names[i] = branches->branch_list[i].branch_name;
        puts(branch_names[i]);
    }

    return branch_names;
}


/*
    Remove the file from tracking in the current branch
    Return the last known hash value,
*/
int svc_rm(void *helper, char *file_name) {
    //printf("call svc_rm with file: %s\n", file_name);
    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

    if (file_name == NULL){
        return -1;
    }

    // check if file exist at the given path
    FILE * f = fopen(file_name, "r");
    // check if file exist at the given path
    if (f == NULL) { // unable to open file
        return -2;
    }

    // find corresponding file
    for (size_t i = 0; i<branch_ptr->n_files; i++) {
        struct svc_file *file = &(branch_ptr->files[i]);

        // find the file in the branch list
        if (strcmp(file->file_path, file_name) == 0) {
            // confirm that this file is being tracked
            if (file->added == 1 && file->removed == 0) {
                file->added = 0; // remove file is same as stoping tracking
                file->removed = 1; // mark the file is removed
                file->staged = 0;
                // return the old hash code
                return file->hash_code;
            }
        }
    }
    // if the given file is not beening tracked
    return -2;
}


/*
    Helper function of search_commit_tracked_files
*/
void restore_file(void *helper, struct svc_file *file,
                  struct svc_file * cf,
                  struct commit * commit) {

  int hash = hash_file(helper, file->file_path);

  // undo changes for the tracked files
  if (hash != cf->hash_code) {
       // undo changes for the tracked files
        char *temp_path = strdup(file->file_path);
        char *file_name = get_file_name(temp_path);
        char *dest_path = file->file_path;

        char *src_path = calloc(1, BUF_LEN);
        strcpy(src_path, commit->dir_name); // copy content
        strcat(src_path, "/");
        strcat(src_path, file_name);

        copy_file_content(dest_path, src_path);
        free(src_path);
        free(temp_path);
        return;
  }
  // update file stages
  file->added = cf->added;
  file->staged = cf->staged;
  file->removed = cf->removed;
  file->modified = cf->modified;
  file->hash_code = hash_file(helper, file->file_path);
  return;
}


/*
    Check if the given file is being tracked within the given commit
    The checked file in the given commits are in:
    added_files, modified_files, or tracked_files

    Return 1 if found.
    Return 0 if cannot find
*/
int search_commit_tracked_files(void *helper, struct svc_file *file,
                                              struct commit * commit){

  int find = 0;
  for (size_t i = 0; i<commit->n_added_files; i++){
    struct svc_file * cf = &commit->added_files[i];
    if (strcmp(cf->file_path, file->file_path) == 0) {
      find = 1;

      // undo changes for the tracked files
      restore_file(helper, file, cf, commit);
    }
  }

  for (size_t i = 0; i<commit->n_modified_files; i++){
    struct svc_file * cf = &commit->modified_files[i];
    if (strcmp(cf->file_path, file->file_path) == 0) {
      find = 1;

      // undo changes for the tracked files
      restore_file(helper, file, cf, commit);
    }
  }

  for (size_t i = 0; i<commit->n_tracked_files; i++){
    struct svc_file * cf = &commit->tracked_files[i];
    if (strcmp(cf->file_path, file->file_path) == 0) {
      find = 1;

      // undo changes for the tracked files
      restore_file(helper, file, cf, commit);
    }
  }
  return find;
}


/*
    Helper function for svc_reset
    Undo all types of changes to the file:
    (only the tracked files of that commit will be reverted)
    1. The tracked files list will be reverted to the one of that commit
    2. The files added after that commit will be removed in current branch
    3. Do nothing to the files added after that commit
    4. Note that these files might still be tracked in another branch
*/
void undo_file_changes(void *helper, struct commit * commit) {

  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);
  for (size_t j = 0; j< branch_ptr->n_files; j ++){

    struct svc_file * file = &branch_ptr->files[j];

    // check if this commit tracks this file
    // undo changes for the tracked files
    int find = search_commit_tracked_files(helper, file, commit);

    // update file hash code
    file->hash_code = hash_file(helper, file->file_path);

    if (find == 0) {
      // The files added after this commit will be not tracked
      file->added = 0;
      file->staged = 0;
      file->modified = 0;
      file->removed = 1;
    }
  }
}


/*
    Helper function for svc_reset
    Clean commit history after the given commit_id on current branch
*/

void clean_commit_history(void *helper, struct commit *commit_histroy,
                          int *n_commit, int commit_idx) {
  *n_commit = commit_idx +1;
  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);
  // update the new commits' number on current active branch
  branch_ptr->n_commit = *n_commit;
}


/*
    Reset the "current" branch to the commit with the id given.
    Disgard any uncommitted changes
*/
int svc_reset(void *helper, char *commit_id) {

  if (commit_id == NULL) {
    return -1;
  }

  // check the given commit_id exits
  // get all historical branches
  struct svc_branch_list *branches = (struct svc_branch_list *)helper;

  // check the given commit_id exits
  for (int i = 0; i<branches->n_branches; i++) {
    struct svc_branch *branch_ptr = &branches->branch_list[i];
    for (int j = 0; j < branch_ptr->n_commit; j++) {
      // check commit_id is created before
      if (strcmp(branch_ptr->commit_histroy[j].commit_id, commit_id) == 0) {
          //printf("line 1152 Will reset to %s\n", commit_id);

          // undo all changes after this commit
          undo_file_changes(helper, &branch_ptr->commit_histroy[j]);

          // clean commit history after the given commit_id on current branch
          clean_commit_history(helper, branch_ptr->commit_histroy,
                                       &branch_ptr->n_commit, j);
          return 0;
      }
    }
  }
  // Invalid commit id
  return -2;
}


/*
    Helper function for svc_merge
    Return -1 if inliad branch_name
    return 0 otherwise
*/
int check_valid_merge_branch_name(void *helper, char *branch_name) {

  if (branch_name == NULL) {
    puts("Invalid branch name");
    return -1;
  }

  // check if the given branch name can be found
  int check = check_branch_exist(helper, branch_name);
  if (check < 0) { // the given branch name doesn't exists
    puts("Branch not found");
    return -1;
  }

  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

  // check if the given name is the currently checked out branch
  if (strcmp(branch_ptr->branch_name, branch_name) == 0) {
    puts("Cannot merge a branch with itself");
    return -1;
  }

  // check if uncommitted changes exists
  // scenario 1: some files are in the staging phrase
  check = check_uncommitted_changes(branch_ptr);
  if (check < 0) {
    puts("Changes must be committed");
    return -1;
  }
  // scenario 2: some tracked files have been modified
  check = check_changes_tracked_file(helper, branch_ptr);
  if (check < 0) {
    puts("Changes must be committed");
    return -1;
  }

  // valid branch name
  return 0;
}


/*
    Helper function for svc_merge
    Restore the given branch to the state of its last commit
*/
void recover_to_last_commit(void *helper, struct svc_branch *branch_ptr) {

  // get its last commit
  struct commit * bran_com_history = branch_ptr->commit_histroy;
  struct commit *last_commit = &bran_com_history[branch_ptr->n_commit-1];

  for (size_t j = 0; j< branch_ptr->n_files; j ++) {

    struct svc_file * file = &branch_ptr->files[j];

    // check if this commit tracks this file
    // undo changes for the tracked files
    search_commit_tracked_files(helper, file, last_commit);
  }
}


/*
    Helper function for svc_merge
    Return the index of resolutions if given file name is found in resolution
    Return -1 if the give file_name is not find in resolution.file_name
*/
int search_in_resolution(char * file_name, struct resolution *resolutions,
                          int n_resolutions) {
  for (int j = 0; j < n_resolutions; j++) {
    char *file_reso_name = resolutions[j].file_name;
    if (strcmp(file_reso_name, file_name) == 0) {
      return j;
    }
  }
  return -1;
}


/*
    Check if the given file_name is on the given branch
    Return the index of that file if found
    Return -1 otherwise
*/
int check_file_in_branch(char *file_name, struct svc_branch *branch_ptr) {

  for (size_t i = 0; i<branch_ptr->n_files; i++) {
    struct svc_file * file = &branch_ptr->files[i];
    if (strcmp(file->file_path, file_name) == 0) {
      return i;
    }
  }
  return -1;
}


/*
    Check and update the files in active branch,
    based on the resolution array given
*/
void check_update_files_w_resolution(void *helper, struct svc_file * file,
                                     struct resolution *resolutions,
                                     int n_resolutions) {
  char *file_name = file->file_path;
  // check if the file is in resolution array
  int index = search_in_resolution(file_name, resolutions, n_resolutions);

  // update file content based on resolution
  if (index >= 0) {
    // update file content that stored within resolution
    char *reso_file_path = resolutions[index].resolved_file;

    if (reso_file_path == NULL) {
      // we need to delete file
      svc_rm(helper, file->file_path);
      file->removed = 1;
      file->added = 0;
    }
    else {
      // update file content based on resolution
      copy_file_content(file->file_path, reso_file_path);
    }
  }
}


/*
    Helper function for svc_merge:
    Check if the given file in the merged branch is included within resolutions
    Return -1 if the given file is not included within resolutions
    Otherwise, rerturn 0
*/
int check_update_files_w_sec_resolution(void *helper, struct svc_file * file,
                                     struct resolution *resolutions,
                                     int n_resolutions) {
  char *file_name = file->file_path;
  // check if the file is in resolution array
  int index = search_in_resolution(file_name, resolutions, n_resolutions);

  // update file content based on resolution
  if (index >= 0) {
    // update file content that stored within resolution
    char *reso_file_path = resolutions[index].resolved_file;

    if (reso_file_path == NULL) {
      svc_rm(helper, file->file_path);
      file->removed = 1;
      file->added = 0;
      return -1;
    }
    else {
      // update file content based on resolution
      copy_file_content(file->file_path, reso_file_path);
      file->added = 1;
      file->staged = 1; //这里不确定
    }
  }
  return 0;
}


/*
  Helper function for svc_merge
*/
struct svc_file * add_to_merge_file_active_branch(void *helper,
                      struct svc_file *src_files, size_t n_src_files,
                      struct resolution *resolutions, int n_resolutions,
                     struct svc_file *merged_files, size_t *n_merged_files) {

    for (size_t i = 0; i<n_src_files; i++) {

      struct svc_file * file = &src_files[i];
      // Check and uodate file concent based on resolution
      check_update_files_w_resolution(helper, file, resolutions, n_resolutions);
      *n_merged_files += 1;
      merged_files = realloc(merged_files,
                            sizeof(struct svc_file) * (*n_merged_files));

      merged_files[*n_merged_files - 1] = *file;
      merged_files[*n_merged_files - 1].file_path = strdup(file->file_path);
    }
  return merged_files;
}


/*
    Heper function of svc_merge:
    Check and merge tracked files of the given branch to the active branch
*/
struct svc_file * add_to_merge_file_second_branch(void *helper,
                      struct svc_branch *branch_ptr, struct svc_file *src_files,
                      size_t n_src_files, struct resolution *resolutions,
                      int n_resolutions, struct svc_file *merged_files,
                      size_t *n_merged_files, struct commit *merge_header,
                      int flag) {

    size_t j = 0;
    while (j < n_src_files) {

      struct svc_file * file = &src_files[j];
      char *file_name = file->file_path;

      // check if this file is also in the current active branch
      int check = check_file_in_branch(file_name, branch_ptr);
      if (check >=0) { // find conflict file, which has been updated
        j++;
        continue;
      }
      else {
        *n_merged_files += 1;
        merged_files = realloc(merged_files,
                              sizeof(struct svc_file) * (*n_merged_files));
        merged_files[*n_merged_files - 1] = *file;
        merged_files[*n_merged_files - 1].file_path = strdup(file->file_path);

        if (flag != 3){ // the file is not removed
          // the file from another branch will be treated as new added file
          // in currently active branch
          merged_files[*n_merged_files - 1].staged = 1;
        }
        j++;
      }
  }
  return merged_files;
}


/*
    Update the file_name in previous commit since we freed the old name memory
    after merge
*/
void update_file_name_in_previous_commit(struct svc_branch *branch_ptr,
                                        int hash, size_t Idx) {

  for (int m = 0; m < branch_ptr->n_commit; m++) {

    // new added file
    struct svc_file *old_file = branch_ptr->commit_histroy[m].added_files;
    for (size_t l = 0; l<branch_ptr->commit_histroy[m].n_added_files; l++) {
      int h1 = old_file[l].hash_code;
      if (h1 == hash) {
        old_file[l].file_path = branch_ptr->files[Idx].file_path;
      }
    }

    // new removed file
    old_file = branch_ptr->commit_histroy[m].removed_files;
    for (size_t l = 0; l<branch_ptr->commit_histroy[m].n_removed_files; l++) {
      int h1 = old_file[l].hash_code;
      if (h1 == hash) {
        old_file[l].file_path = branch_ptr->files[Idx].file_path;
      }
    }

    // tracked and unchanged file
    old_file = branch_ptr->commit_histroy[m].tracked_files;
    for (size_t l = 0; l<branch_ptr->commit_histroy[m].n_tracked_files; l++) {
      int h1 = old_file[l].hash_code;
      if (h1 == hash) {
        old_file[l].file_path = branch_ptr->files[Idx].file_path;
      }
    }

    // modified file
    old_file = branch_ptr->commit_histroy[m].modified_files;
    for (size_t l = 0; l<branch_ptr->commit_histroy[m].n_modified_files; l++) {
      int h1 = old_file[l].hash_code;
      if (h1 == hash) {
        old_file[l].file_path = branch_ptr->files[Idx].file_path;
      }
    }
  }
}


/*
    Helper function for svc_merge:
    Collect the merged file from branches
*/
struct svc_file * collect_merged_files(void * helper,
                    struct svc_branch *branch_ptr,
                    struct svc_file *merged_files, struct commit *active_header,
                    struct commit *merge_header, struct resolution *resolutions,
                    int n_resolutions, size_t *n_merged_files) {

  // back to the last commit of current active branch
  merged_files = add_to_merge_file_active_branch(helper,
                  active_header->added_files, active_header->n_added_files,
                  resolutions, n_resolutions, merged_files, n_merged_files);

  merged_files = add_to_merge_file_active_branch(helper,
                 active_header->modified_files, active_header->n_modified_files,
                 resolutions, n_resolutions, merged_files, n_merged_files);

  merged_files = add_to_merge_file_active_branch(helper,
                  active_header->removed_files, active_header->n_removed_files,
                  resolutions, n_resolutions, merged_files, n_merged_files);

  merged_files = add_to_merge_file_active_branch(helper,
                  active_header->tracked_files, active_header->n_tracked_files,
                  resolutions, n_resolutions, merged_files, n_merged_files);

  // check the merged branch
  merged_files = add_to_merge_file_second_branch(helper,
                    branch_ptr, merge_header->added_files,
                    merge_header->n_added_files,resolutions, n_resolutions,
                    merged_files, n_merged_files, merge_header, 1);

  merged_files = add_to_merge_file_second_branch(helper, branch_ptr,
                    merge_header->modified_files,
                    merge_header->n_modified_files,resolutions, n_resolutions,
                    merged_files, n_merged_files, merge_header, 2);

  merged_files = add_to_merge_file_second_branch(helper,
                    branch_ptr, merge_header->removed_files,
                    merge_header->n_removed_files,resolutions, n_resolutions,
                    merged_files, n_merged_files, merge_header, 3);

  merged_files = add_to_merge_file_second_branch(helper,
                    branch_ptr, merge_header->tracked_files,
                    merge_header->n_tracked_files,resolutions, n_resolutions,
                    merged_files, n_merged_files, merge_header, 4);

  return  merged_files;
}


/*
    Helper function for svc_merge:
    Create new commit for the merge
*/
char * create_merge_commit(void *helper, char *branch_name,
  struct svc_branch *merg_branch_ptr) {

  char merge_info[BUF_LEN];
  strcpy(merge_info, "Merged branch ");
  strcat(merge_info, branch_name);

  // create new commit for this merge
  char *commit_id = svc_commit_noCheck_manually_delete_f(helper, merge_info,
                                                               merg_branch_ptr);
  return commit_id;
}


/*
    Helper function for svc_merge:
    Update the files on current branch based on merge result
*/
void update_branch_files(void *helper, size_t n_merged_files,
                          struct svc_branch *branch_ptr,
                          struct svc_file *merged_files) {

   // Update the branch files
   branch_ptr->n_files = n_merged_files;
   branch_ptr->files = realloc(branch_ptr->files,
                                      sizeof(struct svc_file) * n_merged_files);

  // copy files from merged_files to branch_ptr->files
  for (size_t k = 0; k<branch_ptr->n_files; k++) {
      branch_ptr->files[k] = merged_files[k];
      // update the file_name in previous commits
      int this_hash = branch_ptr->files[k].hash_code;
      update_file_name_in_previous_commit(branch_ptr, this_hash, k);
  }
}


/*
    Helper funtion for svc_commit_noCheck_manually_delete_f
    Add two parents to the new commit
*/
void add_two_parents(void *helper, struct commit *new_commit, char *message,
          struct svc_branch *branch_ptr, struct svc_branch *merg_branch_ptr) {

    // get the lastest commit in current branch
    struct commit *parent_commit;
    struct commit *merg_branch_header;

    // get the lastest commit in current branch
    parent_commit = &branch_ptr->commit_histroy[branch_ptr->n_commit - 1];
    int merg_n_commit = merg_branch_ptr->n_commit;
    merg_branch_header = &merg_branch_ptr->commit_histroy[merg_n_commit - 1];

    // update the parent commit information
    new_commit->n_parent_commit = 2; // can only has 1 parent without merge
    new_commit->parent_commit = realloc(new_commit->parent_commit,
                                        sizeof(struct commit) * 2);
    new_commit->parent_commit[0] = *parent_commit;
    new_commit->parent_commit[1] = *merg_branch_header;

    // Update commit history in current branch
    branch_ptr->n_commit++;
    branch_ptr->commit_histroy = realloc(branch_ptr->commit_histroy,
                                  sizeof(struct commit) * branch_ptr->n_commit);

    merg_branch_ptr = get_merged_branch_ptr(helper,
                                            merg_branch_ptr->branch_name);

    // get new allocation of parent_commit
    if (branch_ptr->n_commit>=2) {
        parent_commit = &branch_ptr->commit_histroy[branch_ptr->n_commit - 2];
    }

    // only add new commit on current branch
    branch_ptr->commit_histroy[branch_ptr->n_commit - 1] = *new_commit;
    merg_n_commit = merg_branch_ptr->n_commit;
    merg_branch_header = &merg_branch_ptr->commit_histroy[merg_n_commit - 1];
    update_overall_commit_list(helper, new_commit, parent_commit);

}


/*:
    Helper function for svc_merge:
    No check for manually deleted files,
    since they will be checked by resolutions array
*/
char *svc_commit_noCheck_manually_delete_f(void *helper, char *message,
                                          struct svc_branch *merg_branch_ptr) {

    struct svc_branch *branch_ptr = get_current_branch_ptr(helper);

    if (message == NULL || branch_ptr->n_files == 0) {
        return NULL;
    }
    // create new commit
    struct commit *new_commit = calloc(1, sizeof(struct commit));

    /* Detect if any changes in the file: add, rm, or modified */
    int valid_commit = dectected_changes_in_f(helper, branch_ptr, new_commit);

    if (valid_commit == 0) {
      free(new_commit->tracked_files);
      free(new_commit);

      return NULL;
    }

    if (valid_commit == 1){

      // calculate the commit ID
      char *commit_id = calloc(1, sizeof(char *));
      commit_id = calculate_commit_id(message, new_commit, commit_id);

      // update new commit's content
      new_commit->message = strdup(message); // store commit message
      new_commit->commit_id = strdup(commit_id); //store commit_id

      // Create temo directory and store tracked file inside the new commit
      store_in_temp_dir(helper, commit_id, new_commit);

      add_two_parents(helper, new_commit, message, branch_ptr, merg_branch_ptr);

      free(commit_id);
      free(new_commit);
      return branch_ptr->commit_histroy[branch_ptr->n_commit - 1].commit_id;
   }

    // if no change since the last commit
    free(new_commit); // free this commit if it cannot be created
    return NULL;
}


/*
    Merge the active header with the header of the branch given
    As long as there is a file being tracked by either branch,
    there is potentially a resolution for it.
*/
char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions,
                int n_resolutions) {

  int check = check_valid_merge_branch_name(helper, branch_name);
  if (check < 0) { return NULL; }

  /* check tracked files in two branches */
  struct svc_branch *branch_ptr = get_current_branch_ptr(helper);
  struct svc_branch *merg_bran_ptr = get_merged_branch_ptr(helper, branch_name);
  int merg_n_commit = merg_bran_ptr->n_commit - 1;
  int bran_n_commit = branch_ptr->n_commit - 1;
  struct commit *merge_header = &merg_bran_ptr->commit_histroy[merg_n_commit];
  struct commit *active_header = &branch_ptr->commit_histroy[bran_n_commit];

  // handle conflicting files
  struct svc_file *merged_files = NULL;
  size_t n_merged_files = 0;

  merged_files = collect_merged_files(helper, branch_ptr,
                      merged_files, active_header, merge_header,
                      resolutions, n_resolutions, &n_merged_files);
  // free old file name
  for (size_t k = 0; k<branch_ptr->n_files; k++) {
    free(branch_ptr->files[k].file_path);
  }

  // update the files on current branch based on merge result
  update_branch_files(helper, n_merged_files, branch_ptr, merged_files);

  free(merged_files);
  // create new commit with merged branch information
  char *commit_id = create_merge_commit(helper, branch_name, merg_bran_ptr);

  puts("Merge successful");
  return commit_id;
}
