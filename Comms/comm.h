#ifndef COMM_H
#define COMM_H

#define MAX_BUFF (1024)
#define NUM_FD (2)

struct coordinator {
    pid_t coor_id;
    int n_process; // the number of subprocess
    struct storage *head; // linked list used to store storage process
};


struct storage {
    int storage_id; /* global id assigned by coordinator */
    pid_t process_id; /* the process id*/
    int read; // communicate channel receive data from coordinator
    int write; // communicate channe send data to coordinator
    struct storage *next;
};

void sighandler(int signo);

int destroy_handler(struct coordinator * coor, int command_id,
                    int command_value);

void exit_handler(struct coordinator * coor);

int toggle_handler(struct coordinator * coor, int command_id);

int store_handler(struct coordinator * coor, int command_id, int command_value);

int get_handler(struct coordinator * coor, int command_id, int command_value);

void lanch_command(struct coordinator * coor);

struct coordinator * coordinator_init();

#endif

