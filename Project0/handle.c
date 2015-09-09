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
 */
void handler(int sig);
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

    while(1==1) {   //&&process still running
      if(interrupt != -1) {
	bytes = write(STDOUT, "Still here\n", 11);
        timeWait.tv_sec = (time_t) 1;
        timeWait.tv_nsec = 0;
      }
      if(bytes != 11)
	exit(-999);
      if(Signal(SIGINT, handler) == SIG_ERR)
      	unix_error("signal error");
      interrupt = nanosleep(&timeWait, &timeInterrupt);
      if(interrupt == -1) {
	timeWait.tv_nsec = timeInterrupt.tv_nsec;
	timeWait.tv_sec = (time_t) 0;
	interrupt = nanosleep(&timeWait, &timeInterrupt);
      }
      /*      if(interrupt ==0) {
        timeWait.tv_sec = (time_t) 1;
        timeWait.tv_nsec = 0;
	}*/
    }
  }
  return 0;
}

//signal handler
void handler(int sig) {
  ssize_t bytes; 
  const int STDOUT = 1; 

  bytes = write(STDOUT, "Nice try.\n", 10); 
  if(bytes != 10) 
    exit(-999);
}


