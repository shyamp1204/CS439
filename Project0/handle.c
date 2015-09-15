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
 *
 *Alex and Katherine driving here
 *
 */
void sigint_handler(int sig);
void handler_SIGUSR1(int sig);

int main(int argc, char **argv)
{
  pid_t pid = getpid();
  if(pid < 0) {
    //error
  }
  else {
    printf("PID = %d\n", pid);
    ssize_t bytes;
    const int STDOUT = 1;
    struct timespec timeWait;
    struct timespec timeInterrupt;
    timeWait.tv_sec = (time_t) 1;
    timeWait.tv_nsec = 0;
    int interrupt = 0;

    while(1) {   //&&process still running
      if(interrupt != -1) {
	bytes = write(STDOUT, "Still here\n", 11);
        timeWait.tv_sec = (time_t) 1;
        timeWait.tv_nsec = 0;
      }

      if(bytes != 11)
	exit(-999);
      if(Signal(SIGINT, sigint_handler) == SIG_ERR)
      	unix_error("signal error");
      if(Signal(SIGUSR1, handler_SIGUSR1) == SIG_ERR)
	unix_error("signal error");

      interrupt = nanosleep(&timeWait, &timeInterrupt);

      while(interrupt == -1) {
	timeWait.tv_nsec = timeInterrupt.tv_nsec;
	timeWait.tv_sec = (time_t) 0;
	interrupt = nanosleep(&timeWait, &timeInterrupt);
      }
    }
  }
  return 0;
}

//signal handler for sigInt
void sigint_handler(int sig) {
  ssize_t bytes; 
  const int STDOUT = 1; 

  bytes = write(STDOUT, "Nice try.\n", 10); 
  if(bytes != 10) 
    exit(-999);
}

//signal handler for SIGUSR1
void handler_SIGUSR1(int sig) {
  ssize_t bytes; 
  const int STDOUT = 1; 

  bytes = write(STDOUT, "exiting\n", 8);
  if(bytes != 8) 
    exit(-999);
  else
    exit(1);
}


