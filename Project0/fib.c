#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);

pid_t first_pid;

/*
 * unix_error - unix-style error routine.
 */
inline static void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
  int arg;
  int print;

  if(argc != 2){
    fprintf(stderr, "Usage: fib <num>\n");
    exit(-1);
  }

  if(argc >= 3){
    print = 1;
  }

  arg = atoi(argv[1]);
  if(arg < 0 || arg > MAX){
    fprintf(stderr, "number must be between 0 and %d\n", MAX);
    exit(-1);
  }

  first_pid = getpid();
  doFib(arg, 1);

  return 0;
}



/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 *
 *Alex and Katherine driving here
 */
pid_t Fork(void);
static void doFib(int n, int doPrint) 
{
  int status;
  int status2;
  pid_t pid;
  pid_t pid2;
  int sum = 0;
  int sum2 = 0;

  if(n == 0)
    exit(0);
  else if(n == 1)
    exit(1);
  else {
    //fork for n-1 value
    pid = Fork();
    if((pid == 0)){
      //in child process
      doFib(n-1, doPrint); 
      exit(n-1);
    }
    
    //fork for n-2 value
    pid2 = Fork();
    if(pid2 == 0) {
      doFib(n-2, doPrint);
      exit(n-2);
    }
  
    // code from B&O book, page 72
    //waiting for an artibrary child process to end, then reap it
    while((pid = waitpid(-1, &status, 0)) > 0) {
      if(WIFEXITED(status)) {
        //child terminated normally
        sum += WEXITSTATUS(status);
      } 
      else
        unix_error("Error with child process! Terminated abnormally");
    }

    // code from B&O book, page 72
    while((pid2 = waitpid(-1, &status2, 0)) > 0) { 
      if(WIFEXITED(status2)) {
        //child terminated normally
        sum2 += WEXITSTATUS(status2);
      } 
      else
        unix_error("Error with 2nd child process! Terminated abnormally");
    }
  }

  //add the two previous fibonacci values together
  int total = sum + sum2;
  if(doPrint && (getpid() == first_pid)) {
    printf("%d\n", total);
  } 
  else
    exit(total);
}

/* 
 *code from B&O book, page 718
 * wrapper class for fork()
 *
 *Alex drove here
 */
pid_t Fork(void) 
{
    pid_t pid;
    if((pid = fork()) < 0) 
      unix_error("Fork error");
    return pid;
}
