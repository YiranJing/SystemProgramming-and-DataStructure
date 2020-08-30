#include "office.h"
#include "queue.h"
#include "stack.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
  Add emp as a new employee to supervisor, update subordinates and n_subordinates
  Employee are created on the heap
*/
void add_new_employee(struct employee* supervisor, struct employee* emp){

  supervisor->n_subordinates += 1;

  supervisor->subordinates = (struct employee*) reallocarray(supervisor->subordinates,
                                                             supervisor->n_subordinates, sizeof(struct employee));

  supervisor->subordinates[supervisor->n_subordinates - 1].name = malloc(sizeof(char)*20);
  strcpy(supervisor->subordinates[supervisor->n_subordinates - 1].name, emp->name);
  supervisor->subordinates[supervisor->n_subordinates - 1].n_subordinates = emp->n_subordinates;
  supervisor->subordinates[supervisor->n_subordinates - 1].subordinates = emp->subordinates;
  supervisor->subordinates[supervisor->n_subordinates - 1].supervisor = supervisor;

}

/*
  Remove emp from supervisor
*/
void remove_employee(struct employee* supervisor, struct employee* emp){

  // search to find the location of emp from subordinates list
  size_t i;
  size_t new_size = 0;
  struct employee* my_subordinates = supervisor->subordinates;

  struct employee* new_subordinates = NULL;
  if (supervisor->n_subordinates-1 > 0) {
    new_subordinates = (struct employee*) malloc(sizeof(struct employee) * (supervisor->n_subordinates-1));
  }


  for (i =0; i<supervisor->n_subordinates; i++){
    struct employee * p = &my_subordinates[i];
    if (p == emp) { // find it, by compare point address
      continue;
    }
    else{
      new_subordinates[new_size] = my_subordinates[i];
      new_size++;
    }
  }

  //free_subordinates(my_subordinates, supervisor->n_subordinates);
  supervisor->n_subordinates--;
  free(emp->name);
  free(emp);
  supervisor->subordinates = new_subordinates;
}




/*
   Add new element to array emplys
*/
void add_emplys(struct employee* emp, struct employee** emplys, size_t* n_employees){

    *n_employees += 1;

    // resize emplys array
    *emplys = (struct employee*) realloc(*emplys, sizeof(struct employee) * (*n_employees));

    (*emplys)[*n_employees - 1].name = malloc(sizeof(char)*20);

	  strcpy((*emplys)[*n_employees - 1].name, emp->name);


    (*emplys)[*n_employees - 1].n_subordinates = emp->n_subordinates;
    (*emplys)[*n_employees - 1].subordinates = emp->subordinates;
    (*emplys)[*n_employees - 1].supervisor = emp->supervisor;

}



/**
 * This function will need to retrieve all employees at a level.
 * A level is defined as distance away from the boss. For example, all
 * subordinates of the boss are 1 level away, subordinates of the boss's subordinates
 * are 2 levels away.
 *
 * if office, n_employees or emplys are NULL, your function must do nothing
 *
 * You will need to provide an allocation for emplys and specify the
 * correct number of employees found in your query.
 */
void office_get_employees_at_level(struct office* off, size_t level,
  struct employee** emplys, size_t* n_employees) {

  // If off, emplys or n_employees is NULL
  if (off == NULL || emplys == NULL || n_employees == NULL){
    return;
  }
  if (level < 0){ // invalid input
    return;
  }

  // initialise queue to store all employees in current level from 0
  struct queue* s = queue_init();
  //struct queue* s1 = queue_init();  // queue to store all employees in the next level

  // my start level is 0
  size_t current_level = 0;
  size_t n_current = 0; // the number of employee in current level
  struct employee* emp = off->department_head;
  // queue s is used to store all elements in current level
  queue_push_copy(s, emp, sizeof(struct employee)); // push boss to queue first

  void* n = NULL;
  while (current_level < level) {

    n_current = s->len;

    for (size_t j = 0; j<n_current; j++) { // pop old off
      n = queue_pop_object(s);
		  struct employee* d = (struct employee*) n;

      // push new employees on queue
      for (size_t i = 0; i< d->n_subordinates; i++){
        struct employee* my_subordinates = d->subordinates;
        queue_push_copy(s, &my_subordinates[i], sizeof(struct employee)); // push to S1
      }
	  }
    current_level++; // update my current level
  }

  n_current = s->len;

  // add all employees of given level to emplys
  for (size_t j = 0; j<n_current; j++) {
    n = queue_pop_object(s);
    emp = (struct employee*) n;
    add_emplys(emp, emplys, n_employees);
  }

	free(s);
	return;

}




/*
  Return the first employee that is not superivising any other employee (top-down, left-to-right).
  A simipler version of Breadth First Traversal
  or Level order traversal
*/
struct employee* get_first_no_superv_emplys(struct office* off){

  struct employee* employ = off->department_head;

  // boss has no exployee
  if (employ->n_subordinates == 0){
    //puts("boss level \n");
    return employ;
  }

  void* n = NULL;
  struct queue* s = queue_init();
  queue_push_copy(s, employ, sizeof(struct employee));

  while (s->len >0){
    n = queue_pop_object(s);
    struct employee* d = (struct employee*) n;
    if (d->n_subordinates == 0){
      queue_free(s);
      return d;
    }
    else{
      for (size_t i = 0; i< d->n_subordinates; i++){
        struct employee* my_subordinates = d->subordinates;
        queue_push_copy(s, &my_subordinates[i], sizeof(struct employee));
      }
    }
  }
}



/**
 * Places an employee within the office, if the supervisor field is NULL
 *  it is assumed the employee will be placed under the next employee that is
 * not superivising any other employee (top-down, left-to-right).
 *
 * If the supervisor is not NULL, it will be added to the supervisor's subordinates list
 *  of employees (make sure the supervisor exists in hierarchy).
 *
 * If the office or employee are null, the function not do anything.
 */
void office_employee_place(struct office* off, struct employee* supervisor,
  struct employee* emp) {

  // Check if the office or employee are null
  if (off == NULL || emp == NULL){
    return;
  }

  // If the supervisor field is NULL
  if (supervisor == NULL){

    // If the department head is null, the new employee should be set as the department head
    if (off->department_head == NULL){

      //printf("Begin emp.n_subordinates %d \n", emp->n_subordinates);
      struct employee* empNew = malloc(sizeof(struct employee));

      // copy the name content to the heap
      empNew->name = malloc(sizeof(char)*20);
	    strcpy(empNew->name, emp->name);
      empNew->subordinates = emp->subordinates;
      empNew->n_subordinates = emp->n_subordinates;
      empNew->supervisor = supervisor;

      off->department_head = empNew;
      return;
    }

    else{ // the department head is not null
      // get the first supervisor that donot have any employee
      struct employee* no_superv_emply = get_first_no_superv_emplys(off);
      add_new_employee(no_superv_emply, emp);
      return;
    }
  }

  // if the supervisor field is not NULL
  else{
    add_new_employee(supervisor, emp);
  }
	return;
}


/*
  Replace emp with its first subordinates as the subordinates of supervisor
  Also remove emp, and free space

  Delete a node from tree
*/
void replace_remove_employee(struct employee* supervisor, struct employee* emp, struct employee* subordinates){

  struct employee* old_subordinates = supervisor->subordinates;
  size_t new_number = (supervisor->n_subordinates - 1) + emp->n_subordinates;


  struct employee* my_subordinates = (struct employee *) malloc(new_number * sizeof(struct employee));

  int find = 0;
  // search emp
  for (size_t i =0; i<supervisor->n_subordinates; i++){
    size_t j = 0;
    struct employee * p = &old_subordinates[i];
    if (emp == p) { // compare pointer address
      //printf("Find repalced %s \n", emp->name);
      free(p->name); // free name!
      find = 1;
      // add subordinate of fired emp to its supervisor
      for (j = 1; j <=emp->n_subordinates; j ++){
        my_subordinates[i+j-1] = emp->subordinates[j-1];
        emp->subordinates[j-1].supervisor = supervisor;
      }
      continue;
    }

    else if (find == 0){
      my_subordinates[i] = old_subordinates[i];
      my_subordinates[i].supervisor = supervisor;
    }
    else { // find = 1
      my_subordinates[i+j] = old_subordinates[i];
      my_subordinates[i].supervisor = supervisor;
    }
}
  supervisor->subordinates = my_subordinates; // update subordinates

  free(old_subordinates);
  free(emp->subordinates);
  return;
}


/**
 * Fires an employee, removing from the office
 * If employee is null, nothing should occur
 * If the employee does not supervise anyone, they will just be removed
 * If the employee is supervising other employees, the first member of that
 *  team will replace him.
 https://edstem.org/courses/3996/discussion/199427
 */
void office_fire_employee(struct employee* employee) {

  if (employee == NULL) {
    return;
  }

  if (employee->n_subordinates == 0) {
    remove_employee(employee->supervisor, employee);

    return;
  }

  // the employee is supervising other employees
  replace_remove_employee(employee->supervisor, employee, employee->subordinates);
	return;
}


/**
 * Retrieves the first encounter where the employee's name is matched to one in the office
 * If the employee does not exist, it must return NULL
 * if office or name are NULL, your function must do nothing
 */
struct employee* office_get_first_employee_with_name(struct office* office,
   const char* name) {

   if (office == NULL || name == NULL){
     return NULL;
   }

  struct employee* final = NULL;
  struct queue* s = queue_init();
  int find = 0;
  struct employee* emp = office->department_head;
  queue_push_copy(s, emp, sizeof(struct employee));
  void* n = NULL;

  while (s->len > 0) {
     n = queue_pop_object(s);
     struct employee* d = (struct employee*) n;

     for (size_t i = 0; i< d->n_subordinates; i++){
         struct employee* my_subordinates = d->subordinates;
         queue_push_copy(s, &my_subordinates[i], sizeof(struct employee));
     }

    int result = strcmp(d->name, name);  // find the employee with the same name
    if (result == 0 && find == 0){
      final = d;
      find = 1;
    }
   }
   free(s);

 	return final;
 }

/**
 * Retrieves the last encounter where the employee's name is matched to one in the office
 * If the employee does not exist, it must return NULL
 * if office or name are NULL, your function must do nothing
 */
struct employee* office_get_last_employee_with_name(struct office* office,
   const char* name) {

   if (office == NULL || name == NULL){
     return NULL;
   }

  struct employee* final = NULL;
  struct queue* s = queue_init();
  struct employee* emp = office->department_head;
  queue_push_copy(s, emp, sizeof(struct employee));
  void* n = NULL;

  while (s->len > 0) {
     n = queue_pop_object(s);
     struct employee* d = (struct employee*) n;

     for (size_t i = 0; i< d->n_subordinates; i++){
         struct employee* my_subordinates = d->subordinates;
         queue_push_copy(s, &my_subordinates[i], sizeof(struct employee));
     }

    int result = strcmp(d->name, name);  // find the employee with the same name
    if (result == 0){
      final = d;
    }
   }
   free(s);

 	return final;
 }


/**
 * Will retrieve a list of employees that match the name given
 * If office, name, emplys or n_employees is NULL, this function should do
 * nothing
 * if office, n_employees, name or emplys are NULL, your function must do
 * nothing.
 * You will need to provide an allocation to emplys and specify the
 * correct number of employees found in your query.
 */
void office_get_employees_by_name(struct office* office, const char* name,
  struct employee** emplys, size_t* n_employees) {

  if (office == NULL || name == NULL || emplys == NULL || n_employees == NULL){
    return;
  }

  struct queue* s = queue_init();
  struct employee* emp = office->department_head;
  queue_push_copy(s, emp, sizeof(struct employee));
  void* n = NULL;

  while (s->len > 0) {
     n = queue_pop_object(s);
     struct employee* d = (struct employee*) n;

     for (size_t i = 0; i< d->n_subordinates; i++){
         struct employee* my_subordinates = d->subordinates;
         queue_push_copy(s, &my_subordinates[i], sizeof(struct employee));
     }

    int result = strcmp(d->name, name);  // find the employee with the same name
    if (result == 0){
      add_emplys(d, emplys, n_employees);
    }
   }
   free(s);
}


/*
  Helper function for office_get_employees_postorder
*/
void post_order_search(struct employee* head, struct employee** emplys, size_t* n_employees) {

  // base case
  if (head->n_subordinates == 0 || head->subordinates == NULL){ // reach leaf 看看是不是可以删掉后面的
    add_emplys(head, emplys, n_employees); // add new employee to emplys
    return;
  }

  // Postorder recursion
  struct employee* my_subordinates = head->subordinates;
  //printf("\nSearch subordinates of %s\n", head->name);

  for (size_t i = 0; i < head->n_subordinates; i++) {
    struct employee * new_emp = &my_subordinates[i];
    //printf("post_order_search %s \n", new_emp->name);  //
    post_order_search(new_emp, emplys, n_employees);

    if (i == (head->n_subordinates-1)) {
      add_emplys(head, emplys, n_employees);
    }
  }

}



/**
 * You will traverse the office and retrieve employees using a postorder traversal
 * If off, emplys or n_employees is NULL, this function should do nothing
 *
 * You will need to provide an allocation to emplys and specify the
 * correct number of employees found in your query.
 */
void office_get_employees_postorder(struct office* off,
  struct employee** emplys,
  size_t* n_employees) {

  // If off, emplys or n_employees is NULL
  if (off == NULL || emplys == NULL || n_employees == NULL){
    return;
  }

  post_order_search(off->department_head, emplys, n_employees); // do recursion for post order traversal
  return;
}


/**
 * The employee will be promoted to the same level as their supervisor and will
 *  join their supervisor's team.
 * If the employee has members on their team, the first employee from that team
 *   will be promoted to manage that team.
 * if emp is NULL, this function will do nothing
 * if the employee is at level 0 or level 1, they cannot be promoted
 */
void office_promote_employee(struct employee* emp) {

}

/**
 * Demotes an employee, placing them under the supervision of another employee.
 * If supervisor or emp is null, nothing should occur
 * If the employee does not supervise anyone, they will not be demoted as they
 *  are already at the lowest position
 * If an employee is to be demoted but their new distance from the boss is less
 *  than the previous position, nothing will happen.
 * Otherwise, the employee should be assigned at the end the supervisor's team
 *  and the first employee from the previously managed team will be promoted.
 *
 * Edge case:
 * if the supervisor use to be an subordinate to the demoted employee
 *   (they will get promoted)
 * the demoted employee will be attached to subordinate's new subordinate's
 *   list not their previous list.
 */
void office_demote_employee(struct employee* supervisor, struct employee* emp){

}

void helper_disband(struct queue* s, struct employee* emp) {

  // queue s is used to store all elements in current level
  queue_push_copy(s, emp->name, sizeof(emp->name)); // push boss to queue first
  //printf("push name %s on queue\n", emp->name);

  // base case
  if (emp->n_subordinates == 0){
    return;
  }

  // recursion
  for (size_t i = 0; i < emp->n_subordinates; i++) {
    helper_disband(s, &(emp->subordinates[i]));
  }

  free(emp->subordinates);
}

/**
 * The office disbands
 * (You will need to free all memory associated with employees attached to
 *   the office and the office itself)
 */
void office_disband(struct office* office) {

   if (office == NULL){
     return;
   }

   if (office->department_head == NULL){
     free(office);
     return;
   }

   // initialise queue to store all employees in current level from 0
   struct queue* s = queue_init();
   struct employee* emp = office->department_head;

   // collect all employee nodes to queue
   void* n = NULL;
   helper_disband(s, emp);

   // free root
   free(emp);

   // free name
   while (s->len > 0) {
     n = queue_pop_object(s);
     char* d = (char*) n;
     //printf("free name %s \n", d);
     free(d);
   }

   free(s);
   free(office);
}
