/* 
 * msh - A mini shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);

/*additional wrapper function declarations*/
pid_t Fork(void);


/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }
   
    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 *
 *code snippets from page 735 of B&O book
 *
 *Alex and Katherine driving here
 *
*/
void eval(char *cmdline) 
{
  int status;
  char *argv[MAXARGS];
  char buf[MAXLINE];
  int bg;
  pid_t pid;
  sigset_t mask;
  pid_t childPID;

  strcpy(buf, cmdline);
  //bg = 1 if true, fg if 0
  bg = parseline(buf, argv);
  if(argv[0] == NULL)
    return;

  if(!builtin_cmd(argv)) { //argv
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    if((childPID=Fork()) == 0) {
      //child process
      //childPID = 0 right now
      setpgid(0,0);
      childPID = getpid();

      sigprocmask(SIG_UNBLOCK, &mask, NULL);

      if(execve(argv[0], argv, environ) < 0) {
	       printf("%s: Command not found.\n", argv[0]);
	       exit(0);
      }
    } else {
      //childPID = child's pid 
      if(!bg) {
        addjob(jobs, childPID, 1, cmdline);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        waitfg(childPID);
      } else {
        //add job to BACKGROUND instead
        //and DO NOT wait for child process to terminate!
        addjob(jobs, childPID, 2, cmdline);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        // if running in background, 
        struct job_t* childJob = getjobpid(jobs, childPID);
        printf("[%i] (%d) %s", (int) (childJob->jid), childPID, cmdline);
        return;
      }
    }
  }
  return;
}



/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
  // printf("ARGV[0] = %s \n", argv[0]);
  if(!strcmp(argv[0], "quit")) {
    exit(0);
  }
  else if(!strcmp(argv[0], "jobs")) {
    //ONLY LIST THE BACKGROUND JOBS!!!
    listjobs(jobs);
    return 1;
  }
  else if(!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
    // printf("IN BG FG IN BUILTIN_CMD\n");
    do_bgfg(argv);
    return 1;
  }
  else if(!strcmp(argv[0], "&")) {
    return 1;
  }
  return 0;     /* not a builtin command */
}


/* 
 * do_bgfg - Execute the builtin bg and fg commands
 * BG <JOB> = CHANGES A STOPPED BACKGROUND JOB TO A RUNNING BACKGROUND JOB!
 * FG <JOB> = CHANGES A STOPPED OR UNNING BACKGROUND JOB TO A RUNNING IN THE FOREGROUND
 */
void do_bgfg(char **argv) 
{
  pid_t pid;
  int jid;
  struct job_t* currentJob;
  char* chArray = argv[1];

  //get pid or jid from argv
  if(isdigit(chArray[0])) {
    pid = (pid_t) atoi(chArray); // at char 0
    currentJob = getjobpid(jobs, pid);
  } else if (chArray[0] == '%') {
    //char* jidSymbol = argv[1];
    //jidSymbol now = char* to argv[1] = "%5"
    //jid = jidSymbol[1];
    jid = (int) atoi(&chArray[1]);

    currentJob = getjobjid(jobs, jid);
  } else {
    printf("argv[1] should contain a pid or jid");
    return;
  }

  //bg and fg jobs - send in SIGCONT signal to job
  kill(-(currentJob->pid), SIGCONT);
  //The bg <job> command restarts <job> by sending it a SIGCONT signal,
  // and then runs it in the background. The <job> argument can be either a PID or a JID.
  if(!strcmp(argv[0], "bg")) {
    //change state of job
    currentJob->state = 2;
    //print all jobs when added to background
    printf("[%i] (%d) %s", (int) (currentJob->jid), (pid_t) (currentJob->pid), currentJob->cmdline);
  } else if(!strcmp(argv[0], "fg")) {
    currentJob->state = 1;
    //wait for foreground job to finish
    waitfg(currentJob->pid);
  }
  return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
  while(pid == fgpid(jobs)) {
    //*************IS THIS SLEEP NECESSARY? WHAT DOES 0 DO? 1 did not work! (test05)
    sleep(0);
  }
  return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
  pid_t pid;
  int status;

  while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {  //&status
    //while waiting for child processes to be reaped, delete job at end?
    int jid = (getjobpid(jobs, pid))->jid;
    //PRINT OUT REAPED CHILD PROCESS INFO
    if(WIFSIGNALED(status)) {
      printf("Job [%i] (%d) terminated by signal %d\n", jid, pid, WTERMSIG(status));
      deletejob(jobs, pid);
    }
    else if (WIFSTOPPED(status)) {
      printf("Job [%i] (%d) stopped by signal %d\n", jid, pid, WSTOPSIG(status));
      //do we make state stop here or in sigtstp_handler??
      struct job_t* stoppedJob = getjobjid(jobs, jid);
      stoppedJob->state = 3;
      return;
    } else {
      deletejob(jobs, pid);
    }
  }
  if(errno != ECHILD) {
    unix_error("sigchld handler error");
  }

  return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
  pid_t currentFGjob = fgpid(jobs);
  if(currentFGjob != 0) {
    kill(-1*currentFGjob, SIGINT);
  }
  return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
  pid_t currentFGpid = fgpid(jobs);
  if(currentFGpid != 0) {
    //stop process, move it to the background
    kill(-1*currentFGpid, SIGTSTP);
  }
  return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}

/* code from B&O book, page 718
 * wrapper class for fork()
 */
pid_t Fork(void) 
{
    pid_t pid;
    if((pid = fork()) <0)
      unix_error("Fork error");
    return pid;
}
