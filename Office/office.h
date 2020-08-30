#ifndef SRC_OFFICE_H_
#define SRC_OFFICE_H_

#include <stdlib.h>

struct employee {
  char* name;
  struct employee* supervisor;
  struct employee* subordinates;
  size_t n_subordinates;
};


struct office {
  struct employee* department_head;
};


void office_employee_place(struct office* off, struct employee* supervisor,
  struct employee* emp);

void office_fire_employee(struct employee* employee);

struct employee* office_get_first_employee_with_name(struct office* office,
  const char* name);

struct employee* office_get_last_employee_with_name(struct office* office,
  const char* name);

void office_get_employees_at_level(struct office* office, size_t level,
  struct employee** emplys, size_t* n_employees);

void office_get_employees_by_name(struct office* office, const char* name,
  struct employee** emplys, size_t* n_employees);

void office_get_employees_postorder(struct office* off, struct employee** emplys,
  size_t* n_employees);

void office_promote_employee(struct employee* emp);

void office_demote_employee(struct employee* supervisor, struct employee* emp);


void office_disband(struct office* office);

#endif
