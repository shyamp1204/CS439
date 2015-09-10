#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv)
{
  pid_t pid;
  if(argc == 2) {
    pid = atoi(argv[1]);  //ascii to int function call
    //send SIGUSR1 signal to pid
    kill(pid, SIGUSR1);
  }
  else  {
    printf("incorrect arguments, please try again\n");
  }
  return 0;
}
