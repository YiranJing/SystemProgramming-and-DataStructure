#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<signal.h>
#include<string.h>
#include<stdio.h>
#include<inttypes.h>
#include"comm.h"



int mutable_state = 1;
int global_id = 0;


void sighandler(int signo) {
    
    if (mutable_state == 0) {
        mutable_state = 1;
        printf("Storage #%d is mutable\n",global_id);
    }
    else {
        mutable_state = 0;
        printf("Storage #%d is immutable\n",global_id);
    }
}


/*
    Helper function for exit command
 */
void exit_handler(struct coordinator * coor){
    char request = 'e';
    struct storage* process = coor->head;
    struct storage* temp = NULL;
    while (process != NULL) {
        temp = process->next;
        write(process->write, &request, sizeof(char)); // send request type
        close(process->write);
        close(process->read);
        kill(process->process_id, SIGKILL); // kill subprocess
        free(process);
        process = temp;
    }
}

/*
    Helper function for toggle command
 */
int toggle_handler(struct coordinator * coor, int command_id) {
    
    if (coor->head == NULL || command_id < 0) {
        return 0;
    }
    
    int valid_id = 0; // check the request id exists
    struct storage* process = coor->head;
    
    while (process != NULL) {
        if (process->storage_id == command_id) {
            valid_id = 1;
            kill(process->process_id, SIGUSR1);
        }
        process = process->next;
    }
    return valid_id;
}


/*
    Check if the given comman_id exists or not
    Send value to the storage process if it is exists
    Return 0 if the command_id not exist
 */
int store_handler(struct coordinator * coor, int command_id, int command_value){
    
    if (coor->head == NULL || command_id < 0) {
        return 0;
    }
    
    int valid_id = 0; // check the request id exists
    struct storage* process = coor->head;
    
    while (process != NULL) {
        if (process->storage_id == command_id) {
            valid_id = 1;
            char request = 's';
            write(process->write, &request, sizeof(char)); // send request type
            write(process->write, &command_value, sizeof(int)); // send data
            break;
        }
        process = process->next;
    }
    return valid_id;
}


/*
    Helper function
    Return 0 if the command_id not exist
 */
int get_handler(struct coordinator * coor, int command_id, int command_value){
    
    if (coor->head == NULL || command_id < 0) {
        return 0;
    }
    
    int valid_id = 0; // check the request id exists
    struct storage* process = coor->head;
    uint32_t n_values = 0;
    uint32_t values[MAX_BUFF]; /* a list of integters */
    while (process != NULL) {
        if (process->storage_id == command_id) {
            valid_id = 1;
            char request = 'g';
            write(process->write, &request, sizeof(char)); // send request type
            read(process->read, &n_values, sizeof(uint32_t)); // read size of data
            //printf("n_values %d\n", n_values);
            read(process->read, values, n_values * sizeof(uint32_t));
            break;
        }
        process = process->next;
    }
    if (valid_id == 0){
        return valid_id;
    }
    // print the integer list
    for (uint32_t i = 0; i < n_values; i++) {
        printf("%d\n", values[i]);
    }
    return valid_id;
}



void lanch_command(struct coordinator * coor) {
    int fd[2]; // parent to child
    pipe(fd);
    int fd2[2]; // child to parent
    pipe(fd2);

    int pid = fork(); // create a storage process by fork the coordinator process

    if (pid == 0) { // child process
        close(fd[1]);
        close(fd2[0]);
        
        uint32_t n_values = 0; /* the length of integers within this storage */
        uint32_t values[MAX_BUFF]; /* a list of integters */
        char request; // the typf of request received from parent
        
        // receive information from parent
        while(1) {
            
            uint32_t integer; // a integer received from coordinator
            read(fd[0], &request, sizeof(char)); // read the type of request
            
            if (request == 's') { // store
                read(fd[0], &integer, sizeof(uint32_t)); //read an integer
                if (mutable_state != 0) { //immutable
                    values[n_values] = integer;
                    n_values++;
                }
            }
            else if (request == 'g') { // get
                // send integer list to client
                write(fd2[1],&n_values,sizeof(uint32_t)); //send the size of data
                write(fd2[1], values, n_values * sizeof(uint32_t)); // send data
            }
            else if (request == 'e') { // exit
                close(fd[0]);
                close(fd2[1]);
            }
        }
    }
    else {// parent process
        close(fd[0]);
        close(fd2[1]);
        // create a new storage process
        struct storage* new_storage = calloc(1, sizeof(struct storage));
        new_storage->storage_id = global_id; 
        new_storage->process_id = pid; // record the pid of child process
        new_storage->write = fd[1]; // used to send data to child
        new_storage->read = fd2[0]; // used to read data from child
        new_storage->next = NULL; // the last stroage
     
        // append the new storage to the linked list
        if (coor->n_process == 0) {
            // the first subprocess
            coor->head = new_storage;
        }
        else {
            // append the new process to the linked list
            struct storage* temp = coor->head;
            while(temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = new_storage;
        }
        // update general information
        coor->n_process++;
        global_id++;
    }
}


/*
    Helper function for desprty
    Return 0 if the command_id not exist
 */
int destroy_handler(struct coordinator * coor, int command_id,
                    int command_value){
    
    if (coor->head == NULL || command_id < 0)
        return 0;
    
    int valid_id = 0; // check the request id exists
    struct storage* current = coor->head;
    struct storage* prev = NULL;
    struct storage* temp = NULL;
    
    // special case: single node
    if (coor->n_process == 1) {
        if  (current->storage_id == command_id) {
            valid_id = 1;
            coor->n_process--;
            close(current->write);
            close(current->read);
            kill(current->process_id, SIGKILL); // kill subprocess
            free(current);
            coor->head = NULL;
        }
        return valid_id;
    }
    // general case
    int num_process = 0;
    while(num_process < coor->n_process && current != NULL
          && current->storage_id != command_id) {
        prev = current;
        current = current->next;
        num_process++;
    }
    if (current != NULL && current->storage_id == command_id)
        valid_id = 1;
    if (valid_id == 1){
        coor->n_process--;
        if (prev == NULL){ // remove the head node
            temp = current->next;
            close(current->write);
            close(current->read);
            kill(current->process_id, SIGKILL); // kill subprocess
            free(current);
            coor->head = temp;
        }
        else {
            prev->next = current->next;
            close(current->write);
            close(current->read);
            kill(current->process_id, SIGKILL); // kill subprocess
            free(current);
        }
    }
    return valid_id;
}





/*
    Initialise the coordinator
*/
struct coordinator * coordinator_init() {
    struct coordinator * coor = calloc(1, sizeof(struct coordinator));
    coor->coor_id = getpid();
    coor->n_process = 0;
    coor->head = NULL; // head of linked list used to store storage process
    return coor;
}


int main () {
    
    struct coordinator * coor = coordinator_init();
    char buf[MAX_BUFF];
    char command[10];
    int command_id;
    int command_value;
    signal(SIGUSR1, sighandler); // by default the list mutable

    while(fgets(buf, MAX_BUFF, stdin)){ // read from user input
        /* Lunch command */
        if (strncmp(buf, "LAUNCH", 6) == 0) {
            lanch_command(coor);
        }
        /* Store */
        else if (strncmp(buf, "STORE", 5) == 0) {
            if (sscanf(buf, "%s %d %d", command, &command_id, &command_value) == 3){
                // check the request id exists, and send data if the id exists
                int valid_id = store_handler(coor, command_id, command_value);
                if (valid_id == 0) {
                    fprintf(stderr, "Unable to process request\n");
                }
            }
            else {
                fprintf(stderr, "Unable to process request\n");
            }
        }
        /* Get */
        else if (strncmp(buf, "GET", 3) == 0) {
            if (sscanf(buf, "%s %d", command, &command_id) == 2) {
                // check the request id exists, and communicate with process
                int valid_id = get_handler(coor, command_id, command_value);
                if (valid_id == 0) {
                    fprintf(stderr, "Unable to process request\n");
                }
            }
            else {
                fprintf(stderr, "Unable to process request\n");
            }
        }
        /* Destory */
        else if (strncmp(buf, "DESTROY", 7) == 0) {
            if (sscanf(buf, "%s %d", command, &command_id) == 2) {
                int valid_id = destroy_handler(coor, command_id, command_value);
                if (valid_id == 0) {
                    fprintf(stderr, "Unable to process request\n");
                }
            }
            else {
                fprintf(stderr, "Unable to process request\n");
            }
        }
        /* Toggle */
        else if (strncmp(buf, "TOGGLE", 6) == 0) {
            if (sscanf(buf, "%s %d", command, &command_id) == 2) {
                int valid_id = toggle_handler(coor, command_id);
                if (valid_id == 0) {
                    fprintf(stderr, "Unable to process request\n");
                }
            }
            else {
                fprintf(stderr, "Unable to process request\n");
            }
        }
        /* EXIT */
        else if (strncmp(buf, "EXIT", 4) == 0) {
            exit_handler(coor);
            free(coor);
            exit(0);
        }
        else{
            fprintf(stderr, "Unable to process request\n");
        }
    }
}


