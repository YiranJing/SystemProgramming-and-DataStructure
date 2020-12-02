#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include "Lab_exercise.h"



// declare global variables
int n; // total number of students
int t; // the time limit for oral exam
int k; // the number of students in each group

int n_current_assigned_id; //  the number of students assigned groupID
bool s_id_available;
int called_gid; // group id is called by teacher
bool room_available;
bool lab_exercise_finish;
int n_groups = 0; // the total number of groups
int n_current_finish = 0; // the number of finished student in current group


pthread_mutex_t id_assign_lock; // mutex for student_id assignment
pthread_mutex_t room_lock; // only one room

pthread_cond_t id_assign_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t id_assign_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t room_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t room_full = PTHREAD_COND_INITIALIZER;

// cond variable during each group taking exam
pthread_cond_t group_barrier = PTHREAD_COND_INITIALIZER;


// calculate group id based on the given student id and K
int calculate_group_id(int n_current_assigned_id) {
    int group_id = (int)((n_current_assigned_id - 1)/ k) + 1;
    return group_id;
}

void *teacher_give_id() {

    //assign ids to students
    for (int local_id = 1; local_id <= n; local_id++) {
        pthread_mutex_lock(&id_assign_lock);
        if (s_id_available == true) {
            pthread_cond_wait(&id_assign_empty, &id_assign_lock);
        }
        //assert(s_id_available == false);
        n_current_assigned_id = local_id;
        int group_id = calculate_group_id(n_current_assigned_id);
        
        if (group_id > n_groups) {
            // print exactly once after create a new group
            n_groups = group_id;
            printf("Teacher: Now I start to assign group id [%d] to students.\n"
                   , group_id);
        }
        
        s_id_available = true;
        pthread_cond_signal(&id_assign_full);
        pthread_mutex_unlock(&id_assign_lock);
    }
    return NULL;
}


void * teacher_routine(void *arg) {

    // assign student id to students;
    teacher_give_id();

    // teacher wait for the last student get his id.
    pthread_mutex_lock(&id_assign_lock);
    if (s_id_available == true) {
        // if blocked, means that the last student havenot finish get ID.
        pthread_cond_wait(&id_assign_empty, &id_assign_lock);
    }
    pthread_mutex_unlock(&id_assign_lock);

    
    int l_id = 0; // local variable used to control the number of groups
    
    /* Intializes random number generator */
    srand(time(0));
    while (l_id < n_groups) {
        
        l_id++; // next group
        pthread_mutex_lock(&room_lock);

        while (room_available == true) {
            pthread_cond_wait(&room_empty, &room_lock);
        }
        assert(room_available == false);

        called_gid = l_id;
        printf("\n\nTeacher: The lab is now available. Students in group ");
        printf("[%d] can enter the room and start your lab exercise.\n", l_id);
        
        room_available = true;
        pthread_cond_broadcast(&room_full); // wake up all waiting student
        
        pthread_mutex_unlock(&room_lock);

        //  calculate the lab time
        int t2 = t / 2 + 1;
        int t_exe = (int)rand() % t2 + t2;
        if (t %2 == 0) {
            t_exe--;
        }
        assert(t_exe <=t);
        assert(t_exe >=t/2);
        
        sleep(t_exe);
        printf("Teacher: Student in group [%d] took %d", l_id, t_exe);
        printf(" units of time to complete the exercise.\n");
        
        // wake up all students doing the exercise
        pthread_mutex_lock(&room_lock);

        lab_exercise_finish = true;
        // all student listen teacher's call
        pthread_cond_broadcast(&group_barrier);
        
        pthread_mutex_unlock(&room_lock);
    }
    return NULL;
}





int students_get_sid() {
    
    // obtain my student id from teacher
    pthread_mutex_lock(&id_assign_lock);
            
    while(s_id_available == false) {
        //puts("wait for teacher give me student Id");
        // unlock mutex before sleeping, waiting for id_assign_lock is available
        pthread_cond_wait(&id_assign_full, &id_assign_lock);
    }
    
   // the current global n_current_assigned_id can be used to calculate group_id
    int my_group_id = calculate_group_id(n_current_assigned_id);
    printf("Student: I am assigned to group [%d].\n", my_group_id);

    s_id_available = false; // protect by muetx
    // wake up teacher, since teacher is waiting for the next submit available
    pthread_cond_signal(&id_assign_empty);
    pthread_mutex_unlock(&id_assign_lock);
    
    return my_group_id;
}





void * students_routine(void *arg) {
    
    // obtain s_id from teacher
    int my_group_id = students_get_sid(); // my_group_id is local variable

    // waiting to be called to enter the room for exam
    pthread_mutex_lock(&room_lock);
    while(room_available == false || my_group_id != called_gid){
        pthread_cond_wait(&room_full, &room_lock);
    }

    //assert(room_available == true);
    
    printf("Student in group [%d]: My group is called by the ", my_group_id);
    printf("teacher and I will enter the lab now.\n");
   
   // students will be blocked until teacher send signal
    while (lab_exercise_finish == false){
        // student within the same group taking exam are waiting here
        pthread_cond_wait(&group_barrier, &room_lock);
    }
    assert(lab_exercise_finish == true);
    
    
    n_current_finish++; // increment from 1 to n
    printf("Student in group [%d]: We have completed our ", my_group_id);
    printf("exercise and leave the lab now.\n");
    
    // make the room available （done by only the last leaving student）
    if((n_current_finish % k) == 0 || n_current_finish == n) {
        room_available = false;
        lab_exercise_finish = false;
        pthread_cond_signal(&room_empty);
    }
        
    pthread_mutex_unlock(&room_lock);

    pthread_exit(EXIT_SUCCESS);
    return NULL;
}







int main(int argc, char ** argv) {

    // ask user provide parameters
    puts("Please enter the total number of students n (int): ");
    scanf("%d", &n);
    puts("Please enter the number of students in each group k (int): ");
    scanf("%d", &k);
    puts("Please enter the time limit for each group to do the exercise: ");
    scanf("%d", &t);

    // create threadid for students and teacher
    pthread_t *student_tid = malloc(n * sizeof(pthread_t));
    pthread_t teacher_tid;


    // launch threads
    if (pthread_create(&teacher_tid, NULL, teacher_routine, NULL) != 0) {
        perror("unable to create thread");
            return 1;
    }
    for (size_t i = 0; i < n; i++) {
        if (pthread_create(&student_tid[i], NULL, students_routine, NULL) != 0) {
            perror("unable to create thread");
            return 1;
        }
    }

    s_id_available = false; // start from teacher （assign student id）
    room_available = false; // start from teacher (assign room)
    lab_exercise_finish = false;

    // wait for threads to finish
    for (size_t i = 0; i < n; i++) {
        if (pthread_join(student_tid[i], NULL) != 0) {
            perror("unable to join thread");
            return 1;
        }
    }
    pthread_join(teacher_tid, NULL);

    printf("\n\nMain thread: All students have completed their lab exercises, ");
    printf("The simulation will end now.\n"); // print by the main thread
    
    //destroy and free memory
    free(student_tid);
    pthread_mutex_destroy(&id_assign_lock);
    pthread_mutex_destroy(&room_lock);
    pthread_cond_destroy(&id_assign_empty);
    pthread_cond_destroy(&id_assign_full);
    pthread_cond_destroy(&room_empty);
    pthread_cond_destroy(&room_full);
    pthread_cond_destroy(&group_barrier);

    return 0;

}






