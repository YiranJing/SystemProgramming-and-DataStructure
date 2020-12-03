# Group Lab Exercise
There are *N* students in the class to do lab exercises. The students are divided into *M* groups, each having *K* students except the last group which may have less than *K* students. Each student is assigned to a group with a group *id* **by the teacher**.

There is only one lab room and each time it can only hold one group of students. The students can enter the lab to conduct their lab exercise only when their group *id* is called by the teacher. Each group can only do the lab exercise once.

The exercise will be limited to *T* units of time for each group. A group may take a minimum of *T*/2 units of time to complete the exercise. After completion, the group will leave the room immediately.

The teacher will call the next group of students to enter the lab after the current group reports the completion of their exercise and the room becomes empty.

**Usage**

    $ make

The program needs three user input:

1. total number of students
2. the time limit for oral exam
3. the number of students in each group

