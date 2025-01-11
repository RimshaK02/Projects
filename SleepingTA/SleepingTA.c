#include <stdio.h>
#include <stdlib.h> // For rand() to randomize programming, asking, and helping times
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/syscall.h> //Only used to print tid with gettid(), not to solve anything in the assignment

pid_t gettid(){
  return syscall(SYS_gettid);
}

#define NUM_STUDENTS 5
#define MAX_PROGRAMMING_TIME 5
#define MAX_ASKING_TIME 5
#define MAX_HELPING_TIME 5
#define TRUE 1 //To make code clearer
#define FALSE 0 //To make code clearer

//Function Prototypes
void *student_runner(void *param); //Will be run by each student thread
void *TA_runner(void *param); //Will be run by TA thread

//Variables
pthread_t tid;
pthread_attr_t attr;
pthread_t workers[NUM_STUDENTS];
pthread_t chairs[3];
sem_t TA_Available;
sem_t Student_Done_Asking;
sem_t TA_Done_Helping;
sem_t TA_Sleeping;
sem_t chair1;
sem_t chair2;
sem_t chair3;
int chairs_full = 0;
int TA_ASLEEP = TRUE;
pthread_mutex_t chair_status_mutex;


int main(int agrc, char *argv[]){
  
  //Initialize chair semaphores
  if (sem_init(&chair1,0,1) != 0 || sem_init(&chair2,0,1) != 0 || sem_init(&chair3,0,1) != 0) {
    printf("ERROR: Semaphore failed to initialize!\n");
    return -1;
  }
  
  //Initialize chair status mutex (Used for chnging value of shared boolean chairs_full)
  if (pthread_mutex_init(&chair_status_mutex, NULL)!=0) {
    printf("ERROR: Mutex failed to initialize!\n");
    return -1;
  }
  
  //Initialize other semaphores
  if (sem_init(&TA_Available,0,1) != 0 || sem_init(&Student_Done_Asking,0,0) != 0 || sem_init(&TA_Done_Helping,0,0) != 0 || sem_init(&TA_Sleeping,0,0) != 0) {
    printf("ERROR: Semaphore failed to initialize!\n");
    return -1;
  }
  
  //Initialize pthread attributes
  if (pthread_attr_init(&attr) != 0) {
    printf("ERROR: Pthread attributes failed to initialize!\n");
    return -1;
  }
  
  //Create student threads and add them to workers array
  for (int i=0; i<NUM_STUDENTS; i++){
    if (pthread_create(&tid, &attr, student_runner, NULL) != 0) {
      printf("ERROR! Student thread failed to create!\n");
      return -1;
    }
    workers[i] = tid;
  }
  
  //Create TA thread
  if (pthread_create(&tid, &attr, TA_runner, NULL) != 0) {
    printf("ERROR! Student thread failed to create!\n");
    return -1;
  }

  //Wait for all student threads to terminate
  for (int j=0; j<NUM_STUDENTS; j++){
    pthread_join(workers[j], NULL);
  }
  
  //Wait for TA thread to terminate
  pthread_join(tid,NULL);

  return 0;
}

//Function that will be run by student threads
void *student_runner(void *param){

  //Infinite loop of programming and asking
  while (TRUE) {
    //Student Programming
    int program_time;
    program_time = (rand()%MAX_PROGRAMMING_TIME) + 1; // Random time between 1 and MAX_PROGRAMMING_TIME seconds
    printf("Student %d programming for %d seconds\n", gettid(), program_time);
    sleep(program_time);
      
    //Student Seeking Help From TA
    pthread_mutex_lock(&chair_status_mutex); //Lock shared boolean to read its value
    if (!(chairs_full == 3)){
        //If at least one chair is available
        pthread_mutex_unlock(&chair_status_mutex); //Done reading variable -> unlock
        /*
        Chairs are handled as if they were a queue, to ensure first come first serve order.
        Chair 1 is the closest to the TA's office (first in line), chair 3 is the furthest (last in line)
        */
        sem_wait(&chair3); //Sit at chair 3 (This line will not be hit if seats full) and increments waiting students
        pthread_mutex_lock(&chair_status_mutex); //Lock shared boolean to change its value
        chairs_full +=1; //Increments waiting students 
        printf("Student %d sat on chair 3, waiting=%d\n", gettid(), chairs_full);
        pthread_mutex_unlock(&chair_status_mutex); //Done changing variable -> unlock
        sem_wait(&chair2); //Student in chair 3 waits to be able to move up to chair 2 (if empty, moves immediately)
        printf("Student %d moved to chair 2\n", gettid());
        sem_post(&chair3); //Declares chair 3 empty, allowing next student to take it
        sem_wait(&chair1); //Student in chair 2 waits to move up to chair 1 (if empty, moves immediately)
        printf("Student %d moved to chair 1\n", gettid());
        sem_post(&chair2); //Declares chair 2 empty, allowing next student to take it
        sem_wait(&TA_Available); //Waits until TA available (not necessarily awake, just until previous student is done)
        pthread_mutex_lock(&chair_status_mutex);
        chairs_full-=1; //Decrements waiting students 
        printf("Student %d entered TA office, waiting = %d\n", gettid(), chairs_full);
        pthread_mutex_unlock(&chair_status_mutex);
        sem_post(&chair1); //Declares chair 1 empty, allowing next student to take it

        
        if (TA_ASLEEP) {
            /*
            TA sleeping is modelled by a semaphore that is empty. TA will be waiting on this semaphore
            Once a student wants to wake up that TA, they post that semaphore which will immediately be
            decremented by the TA, waking them up.
            */
            printf("Student %d waking up TA\n", gettid());
            sem_post(&TA_Sleeping);
        }
        while (TA_ASLEEP); // Student doesnt begin answering questoins until TA wakes up
        int ask_time;
        ask_time = (rand()%MAX_ASKING_TIME) + 1; // Random time between 1 and MAX_ASKING_TIME seconds
        printf("Student %d Asking question for %d seconds\n", gettid(), ask_time);
        sleep(ask_time); //Student asking question
        sem_post(&Student_Done_Asking); //Student is finished asking, allowing TA to start answering
        sem_wait(&TA_Done_Helping); //Wait for TA to finish answering
	sem_post(&TA_Available); //Leave office, indicating to the next student that they can come in
        continue; //Leave and continue the loop (start programming again)
    } else {
        //If no chairs available
        printf("Student %d will try later\n", gettid());
        pthread_mutex_unlock(&chair_status_mutex); //Done reading variable -> unlock
        continue; //Leave and continue the loop (start programming again)
    }  
  }
  pthread_exit(0);
}

//Function that will be run by TA thread
void *TA_runner(void *param) {
    while (TRUE) {
        if (chairs_full == 0) {
            //If no students waiting, sleep until semaphore posted again by student
            printf("TA is sleeping\n");
            TA_ASLEEP = TRUE;
            sem_wait(&TA_Sleeping); // Wait until a student wakes up the TA
            TA_ASLEEP = FALSE;
            printf("TA has been woken up by a student\n");
        }

        // Helping students
        sem_wait(&Student_Done_Asking);
        int help_time;
        help_time = (rand()%MAX_HELPING_TIME) + 1; // Random time between 1 and MAX_HELPING_TIME seconds
        printf("TA is helping a student for %d seconds, waiting = %d\n", help_time, chairs_full);
        sleep(help_time); // Time spent helping
	printf("TA has finished helping current student\n");
	sem_post(&TA_Done_Helping);
    }
    pthread_exit(0);
}
