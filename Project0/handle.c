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
 */
int main(int argc, char **argv)
{
  pid_t pid = getpid();
  if(pid < 0) {
    //error
  }
  else {
    printf("PID = %d\n", pid);
    int time = 0;
    ssize_t bytes;
    const int STDOUT = 1;
   
    while(time <5) { // &&process still running
      bytes = write(STDOUT, "Still here   \n", 10);
      time++;
      if(bytes != 10)
	exit(-999);
    }
  }
  return 0;
}


