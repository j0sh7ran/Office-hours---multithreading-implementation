/*
  *Name: Joshua Tran
  *ID: 1001296598
*/
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

//the classes//
#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3

//semaphore declarations//
sem_t Seat_check; //semaphore to check if theres a seat open
sem_t break_check; //semaphore to check if the professor is on a break


//global variables//
static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0; /* total number of students since break */
static int classType;            /*  checks which class is in session */
static int startClass = 1; /* when the first class is in session this will decrement to 0
                              and will help handle to determine which class is in session */
static int students_since_switch; /* total number of students since switching classes */

//student info structure//
typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.
 */
static int initialize(student_info *si, char *filename) // given from github, 1 change
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;
  students_since_switch = 0; // ONLY CHANGE HERE // initializes the students since switch //
 
  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() //given from github, untouched
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1) 
  {
    /*checks to see if the professor needs a break*/
    if(students_since_break == professor_LIMIT)
    {
      //printf("HIT\n"); //DEBUGGING
      sem_wait(&break_check); //has other threads wait until the break is done
      sleep(10); // finishes answering questions just incase there's still a student in office
      take_break(); //professor takes a break
      sem_post(&break_check);// professor returns to office
      printf("professor is returning from break.\n"); // added as both debug and so that we can see when the professor is back
    }


    /* checks to see if we need to give the other class's students a turn, */
    /* then switches the class and re-initializes the students_since_switch. */
    if(students_since_switch == 5)
    {
      if(classType == CLASSA)
      {
        classType = CLASSB;
        students_since_switch = 0;
        //printf("now helping Class B.\n"); // debug statement
      }else if(classType == CLASSB)
      {
        classType = CLASSA;
        students_since_switch = 0;
        //printf("now helping Class A.\n"); //debug statement
      }
    }
    
  }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * checks if there is a seat open for the student, then
 * allows a class A student if there isnt student in office or if class A students exclusively are in office.
 */
void classa_enter() 
{
  int done = 1;
  while(done)
  {
    sem_wait(&break_check); // checks to see if the professor is on a break
    sem_post(&break_check);
  
    // initializes what the starting class is
    // first come first class
    if(startClass == 1)
    {
      classType = CLASSA;
      startClass = 0;
      printf("first class in is A\n");
    }
    if((classType == CLASSA && classb_inoffice == 0) || (startClass == 0 && classb_inoffice == 0))
    {
      if(classType == CLASSB)
      {
        classType = CLASSA;
        students_since_switch = 0;
      }
      sem_wait(&Seat_check); // checks to see if there is a seat open, if so student takes it
      students_in_office += 1;
      students_since_break += 1;
      students_since_switch += 1;
      //printf("students since switch: %d\n", students_since_switch); // debug statement
      classa_inoffice += 1;
      done = 0;
    }
    
  }
  

}

/* Code executed by a class B student to enter the office.
 * checks if there is a seat open for the student, then
 * allows a class B student if there isnt student in office or if class b students exclusively are in office.
 */
void classb_enter() 
{
  int done = 1; //helps leave the while loop below when this function for the thread is done
  while(done)
  {
    sem_wait(&break_check);// checks to see if the professor is on a break
    sem_post(&break_check);// imediately posted so the critical area isn't affected

    // initializes what the starting class is
    // first come first class
    if(startClass == 1)
    {
      classType = CLASSB;
      startClass = 0;
      printf("first class in is B\n");
    }
    if((classType == CLASSB && classa_inoffice == 0) || (startClass == 0 && classa_inoffice == 0))
    {
      if(classType == CLASSA)
      {
        classType = CLASSB;
        students_since_switch = 0;
      }
      sem_wait(&Seat_check); // checks to see if there is a seat open, if so student takes it
      students_in_office += 1;
      students_since_break += 1;
      students_since_switch += 1;
      //printf("students since switch: %d\n", students_since_switch); // debug statement
      classb_inoffice += 1;
      done = 0;
    }
    
  }

}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) //given from github, untouched
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * decrements the num students in office and for that class
 * releases seat_check semaphore for another student
 */
static void classa_leave() 
{
  students_in_office -= 1;
  classa_inoffice -= 1;
  sem_post(&Seat_check); // opens up a seat when the student leaves
}

/* Code executed by a class B student when leaving the office.
 * decrements the num students in office and for that class
 * releases seat_check semaphore for another student
 */
static void classb_leave() 
{
  students_in_office -= 1;
  classb_inoffice -= 1;
  sem_post(&Seat_check); // opens up a seat when the student leaves
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) //given from github, untouched
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) // given from github, untouched
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  /* initializing semaphores */
  sem_init(&break_check, 0, 1); // initialize the break check semaphore for professor thread
  sem_init(&Seat_check, 0, 3); // initialize the seat check semaphore for 3 seats

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {
    /* Grabs student info and starts a thread for it*/
    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);          
    student_type = random() % 2;
    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    /*ERROR CHECKING*/
    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  /* semaphore cleanup */
  sem_destroy(&Seat_check);
  sem_destroy(&break_check);

  printf("Office hour simulation done.\n");

  return 0;
}
