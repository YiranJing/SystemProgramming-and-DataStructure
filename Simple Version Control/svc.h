#ifndef svc_h
#define svc_h

#include <stdlib.h>
#include <stdio.h>
#include <ftw.h>

#define COMMIT_LENGTH (6)
#define BUF_LEN (255)
#define TEMP_PATH (12)
#define CHAR_LENGTH (2)
#define  _POSIX_C_SOURCE 200809L
#define  _XOPEN_SOURCE 500L


typedef unsigned char BYTE;

/*
  Structure for each file
*/
struct svc_file {
    int hash_code;
    char *file_path;
    int added; // = 1 after been added to svc
    int staged; // =1 if in the staged phrase.
    int removed; // = 1 after been removed to svc
    int modified; // = 1 by checking hash value
};

/*
  Structure for each branch
*/
struct svc_branch {
    char *branch_name;
    struct svc_file *files; // a list of files, might not be tracked.
    size_t n_files; // number of files (length of files)
    struct commit *commit_histroy; // the list of commits on this branch
    int n_commit; // length of commit_histroy;
    int valid; // valid = 0 if it is removed from svc
};

/*
  Structure for each commit
*/
struct commit {
    struct svc_branch *helper;
    char *commit_id;
    char *message;
    struct svc_file *added_files; // a list of new added file in this commit
    size_t n_added_files; // length of added_files
    struct svc_file *removed_files; // a list of deleted file in this commit
    size_t n_removed_files; // length of removed_files
    struct svc_file *modified_files; // a list of modified file in the commit
    size_t n_modified_files; // length of modified_files
    int *modified_old_hash; // a list to records the hash code of changed files
    struct svc_file *tracked_files; // a list of unchanged tracked files
    size_t n_tracked_files;  /* length of tracked_files */
    char *dir_name; // directory name of copied files for the given commit
    struct commit *parent_commit;
    int n_parent_commit; // the number of "parent commit"

};

/*
  Object to memorize all branches and commits
*/
struct svc_branch_list {
    struct svc_branch *branch_list;
    int n_branches; // the number of all historical created branches
    int current_position; // the index of current branch
    struct commit *commit_list; // store all historicial commits
    int n_commit; // length of commit_list
};


typedef struct resolution {
    char *file_name;
    char *resolved_file;
} resolution;

void *svc_init(void);

void cleanup(void *helper);

int hash_file(void *helper, char *file_path);

char *svc_commit(void *helper, char *message);

void *get_commit(void *helper, char *commit_id);

char **get_prev_commits(void *helper, void *commit, int *n_prev);

void print_commit(void *helper, char *commit_id);

int svc_branch(void *helper, char *branch_name);

int svc_checkout(void *helper, char *branch_name);

char **list_branches(void *helper, int *n_branches);

int svc_add(void *helper, char *file_name);

int svc_rm(void *helper, char *file_name);

int svc_reset(void *helper, char *commit_id);

char *svc_merge(void *helper, char *branch_name, resolution *resolutions,
                int n_resolutions);

struct svc_branch * get_current_branch_ptr(void *helper);

struct svc_branch * get_merged_branch_ptr(void *helper, char *branch_name);

int match_file_name(char *file_name, struct commit *new_commit);

char *calculate_commit_id(char *message, struct commit *new_commit,
                          char *commit_id);

int count_trancked_f(struct commit *new_commit);

void add_removed_file_to_commit(void *helper, struct svc_file *file,
                                struct commit *new_commit,
                                struct svc_branch *branch_ptr,
                                int *valid_commit);

void store_tracked_f(void *helper, char *dir_name, char *commit_id,
                      struct commit *commit);

int check_manually_removed_file(void *helper, struct svc_file *file,
                                struct commit *new_commit, int *valid_commit);

void update_overall_commit_list(void *helper, struct commit * new_commit,
                                struct commit * parent_commit);

void undo_file_changes(void *helper, struct commit * commit);

char *svc_commit_noCheck_manually_delete_f(void *helper, char *message,
                                          struct svc_branch *merg_branch_ptr);

#endif
