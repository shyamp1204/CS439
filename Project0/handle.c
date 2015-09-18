#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 *
 * Alex and Katherine driving here
 *
 */
void sigint_handler(int sig);
void handler_SIGUSR1(int sig);

int main(int argc, char **argv)
{
  pid_t pid = getpid();
  if(pid < 0){
    //error
  }
  else {
    printf("PID = %d\n", pid);
    ssize_t bytes;
    struct timespec timeWait;
    struct timespec timeInterrupt;
    timeWait.tv_sec = (time_t) 1;
    timeWait.tv_nsec = 0;
    int interrupt = 0;

    //busy loop that prints "still here every 1 second
    while(1) {
      if(interrupt != -1) {
	bytes = write(1, "Still here\n", 11);
        timeWait.tv_sec = (time_t) 1;
        timeWait.tv_nsec = 0;
      }
      if(bytes != 11)
	exit(-999);
  
      //declare signal handlers to catch the different signals
      //check to make sure they return without an error
      if(Signal(SIGINT, sigint_handler) == SIG_ERR)
      	unix_error("signal error");
      if(Signal(SIGUSR1, handler_SIGUSR1) == SIG_ERR) 
	unix_error("signal error");

      interrupt = nanosleep(&timeWait, &timeInterrupt);

      //if there was an interupt between the 1 second wait, enter this while loop
      while(interrupt == -1) {
	//restore values of time left over from when the interupt was called
	timeWait.tv_nsec = timeInterrupt.tv_nsec;
	timeWait.tv_sec = (time_t) 0;
	interrupt = nanosleep(&timeWait, &timeInterrupt);
      }
    }
  }
  return 0;
}

/*
 *signal handler for sigInt
 *Alex driving here
 */
void sigint_handler(int sig) 
{
  ssize_t bytes; 
  bytes = write(1, "Nice try.\n", 10); 
  if(bytes != 10) {
    exit(-999);
  }
}

/*
 *signal handler for SIGUSR1
 *Alex driving here
 */
void handler_SIGUSR1(int sig) 
{
  ssize_t bytes;
  bytes = write(1, "exiting\n", 8);
  if(bytes != 8) {
    exit(-999);
  }
  else {
    exit(1);
  }
}


