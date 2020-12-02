#ifndef Lab_Exercise
#define Lab_Exercise

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>


int calculate_group_id(int n_current_assigned_id);
void *teacher_give_id();
void * teacher_routine(void *arg);
int students_get_sid();
void * students_routine(void *arg);

#endif

