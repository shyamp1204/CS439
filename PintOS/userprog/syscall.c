#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <lib/user/syscall.h>
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include <lib/kernel/list.h>
#include "devices/shutdown.h"
#include "process.h"
#include "filesys/filesys.h"

//#include "filesys/file.c"


static void syscall_handler (struct intr_frame *);
static int invalid_ptr (void *ptr);
static void my_halt (void);
static void my_exit (struct intr_frame *f);
static void my_exec (struct intr_frame *f); //file is same as cmd_line
static void my_wait (struct intr_frame *f);
static void my_create (struct intr_frame *f);
static void my_remove (struct intr_frame *f);
static void my_open (struct intr_frame *f);
static void my_filesize (struct intr_frame *f);
static void my_read (struct intr_frame *f);
static void my_write (struct intr_frame *f);
static void my_seek (struct intr_frame *f);
static void my_tell (struct intr_frame *f);
static void my_close (int fd);
static struct file *get_file (int fd);
static int next_fd (struct thread *cur);



void
syscall_init (void) 
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  if (invalid_ptr (f->esp))
  {
 	printf("invalid pointer");
  	my_exit (f);
    return;
  }
      
  int syscall_num = *((int *)f->esp);
  //printf("### SYSCALL: %d  ", syscall_num);

	switch (syscall_num)
  {
  case SYS_HALT:
  	printf("### Calling Halt");
		my_halt ();
		break;
	case SYS_EXIT:
		printf("### Calling Exit");
		my_exit (f);
		break;
	case SYS_EXEC:
		printf("### Calling Exec");
		my_exec (f);
		break;
	case SYS_WAIT:
		printf("### Calling Wait");
		my_wait (f);
		break;
	case SYS_CREATE:
		printf("### Calling create");
		my_create (f);
		break;
	case SYS_REMOVE:
		printf("### Calling Remove");
		my_remove (f);
		break;
	case SYS_OPEN:
		printf("### Calling Open");
		my_open (f);
		break;
	case SYS_FILESIZE:
		printf("### Calling FileSize");
		my_filesize (f);
		break;
	case SYS_READ:
		printf("### Calling Read");
		my_read (f);
		break;
	case SYS_WRITE:
		//printf("### Calling Write");
		my_write (f);
		break;
	case SYS_SEEK:
		printf("### Calling Seek");
		my_seek (f);
		break;
	case SYS_TELL:
		printf("### Calling Tell");
		my_tell (f);
		break;
	case SYS_CLOSE:
		printf("### Calling Close");
		my_close (*((int *)(4+(f->esp))));
		break;
	default :
		printf ("Invalid system call! #%d\n", syscall_num);
		thread_exit();   //Exit???  
		break;
    }
}

/*
the user can pass a null pointer, a pointer to unmapped virtual memory,
or a pointer to kernel virtual address space (above PHYS_BASE). All of 
these types of invalid pointers must be rejected without harm to the kernel
or other running processes, by terminating the offending process and 
freeing its resources.

If you encounter an invalid user pointer afterward, you must still be sure
to release the lock or free the page of memory.

FUNCTION TO HANDLE INVALID MEMORY ADDRESS POINTERS FROM USER CALLS
*/
static int 
invalid_ptr (void *ptr) {
	if (ptr == NULL) {
		return 1;
	}
	if (!is_user_vaddr (ptr)) {
		return 1;
	}
	else if (pagedir_get_page (thread_current ()->pagedir, ptr) == NULL) {
		return 1;
	}
return 0;
}

/*
Terminates Pintos by calling shutdown_power_off() (declared in 
devices/shutdown.h).
This should be seldom used, because you lose some information about possible 
deadlock situations, etc. 
*/
static void 
my_halt (void) {
	printf("  ### In Halt\n");
	shutdown_power_off();
}

/*
Whenever a user process terminates, because it called exit or for any other 
reason, print the process's name and exit code, formatted as if printed by 
printf ("%s: exit(%d)\n", ...);. The name printed should be the full name passed
to process_execute(), omitting command-line arguments. Do not print these 
messages when a kernel thread that is not a user process terminates, or when 
the halt system call is invoked. The message is optional when a process fails 
to load.
 
Terminates the current user program, returning status to the kernel. If 
the process's parent waits for it (see below), this is the status that will 
be returned. Conventionally, a status of 0 indicates success and nonzero values
indicate errors.
*/
static void 
my_exit (struct intr_frame *f) {
	printf("  ### In exit\n");
	struct thread *cur = thread_current ();


	int e_status;
  //if (invalid_ptr ((int*)(f->esp)+1))
  if (invalid_ptr ((int*)(4+f->esp)))
  	e_status = -1;
	else 
		e_status = *((int *)(4+f->esp));
		//e_status = *((int *)(f->esp)+1);

	//if user process terminates, print process' name and exit code
	printf("%s: exit(%d)\n", thread_name (), e_status);

	// close all open files

	// Need a lock ?
	while (0)  //there are still open files  //next_fd(cur) == 0;
  {
  // 	printf("+++ list of FILES is not empty\n");

		// //GET NEXT OPEN FILE and close it
		// int i;
		// for (i = 0; i <128; i++) {
		// 	struct file *temp_file = cur->open_files[i];
		// 	if (temp_file != NULL) {
		// 		my_close (i);  		//close the file
  //  			//free (temp_file);  ???
		// 	}
		// }
  }
	// Need to release the lock?

	//EXIT ALL CHILDREN?  no, orphan them
	//RELEASE ALL LOCKS WE ARE HOLDING?
	//SEMA UP IF SOMETHING IS WAITING ON IT?

	//add the status to the tid
	cur->exit_status = e_status;
	//return value in eax
	f->eax = e_status;

	thread_exit ();
}

/* file is same as cmd_line

Runs the executable whose name is given in cmd_line, passing any given 
arguments, and returns the new process's program id (pid). Must return pid -1,
which otherwise should not be a valid pid, if the program cannot load or run 
for any reason. Thus, the parent process cannot return from the exec until it
knows whether the child process successfully loaded its executable. You must 
use appropriate synchronization to ensure this.
*/
static void 
my_exec (struct intr_frame *f)  {
	printf("  ### In exec\n");

	const char *filename = (char *)(4+(f->esp));
	struct thread *cur = thread_current ();

	//CHECK TO MAKE SURE THE FILE POINTER IS VALID
	//if (invalid_ptr (file)) {
    //Exit???
  //  return;	
	//}
  
  tid_t pid = process_execute (filename);
 
  if (pid == TID_ERROR)
  {
    f->eax = -1;
    return;
  }

  //GET THE CHILD PROCESS FROM ITS PID.  HOW DO WE GET THE CHILD???

  //PUT NEW EXEC'ED PROCESS ON CURRENTS CHILDREN LIST
 	//list_push_back (&cur->children_list, &child_thread->child_of);

  f->eax = pid;			//return pid_t;
} 

/*
Waits for a child process pid and retrieves the child's exit status.
If pid is still alive, waits until it terminates. Then, returns the status that 
pid passed to exit. If pid did not call exit(), but was terminated by the kernel 
(e.g. killed due to an exception), wait(pid) must return -1. It is perfectly 
legal for a parent process to wait for child processes that have already 
terminated by the time the parent calls wait, but the kernel must still allow 
the parent to retrieve its child's exit status or learn that the child was 
terminated by the kernel.

wait must fail and return -1 immediately if any of the following conditions are 
true:

pid does not refer to a direct child of the calling process. pid is a direct 
child of the calling process if and only if the calling process received pid as 
a return value from a successful call to exec.
Note that children are not inherited: if A spawns child B and B spawns child 
process C, then A cannot wait for C, even if B is dead. A call to wait(C) by 
process A must fail. Similarly, orphaned processes are not assigned to a new 
parent if their parent process exits before they do.

The process that calls wait has already called wait on pid. That is, a process 
may wait for any given child at most once.
Processes may spawn any number of children, wait for them in any order, and may 
even exit without having waited for some or all of their children. Your design 
should consider all the ways in which waits can occur. All of a process's 
resources, including its struct thread, must be freed whether its parent ever 
waits for it or not, and regardless of whether the child exits before or after 
its parent.

You must ensure that Pintos does not terminate until the initial process exits. 
The supplied Pintos code tries to do this by calling process_wait() 
(in userprog/process.c) from main() (in threads/init.c). We suggest that you 
implement process_wait() according to the comment at the top of the function and 
then implement the wait system call in terms of process_wait().

Implementing this system call requires considerably more work than any of the 
rest.
 */
static void 
my_wait (struct intr_frame *f)  {
	printf("  ### In wait\n");
	pid_t pid = *((int *)(4+(f->esp)));

	struct thread *cur = thread_current ();

  struct list_elem *temp_thread;

  // Need to lock
  for (temp_thread = list_begin (&cur->children_list); temp_thread != list_end (&cur->children_list);temp_thread = list_next (temp_thread)) {
  	struct thread *t = list_entry (temp_thread, struct thread, child_of);
  	if (((int)(t->tid))  == ((int) pid)) {
  		f->eax = process_wait (pid);
	  	return;
  	}
  }
  f->eax = -1;		//return int;
}

/*
Creates a new file called file initially initial_size bytes in size. Returns 
true if successful, false otherwise. Creating a new file does not open it: o
pening the new file is a separate operation which would require a open system 
call. */
static void 
my_create (struct intr_frame *f) {
	printf("  ### In create\n");

	const char *filename = (char *)(4+(f->esp));
	unsigned initial_size = *((int *)(8+(f->esp)));

	struct thread *cur = thread_current ();

 	//if (invalid_ptr(cur_file))
 	//{
		//exit???
    //return;
 	//}

	f->eax = filesys_create (filename, initial_size);   //returns boolean
}

/*
Deletes the file called file. Returns true if successful, false otherwise. 
A file may be removed regardless of whether it is open or closed, and 
removing an open file does not close it. */
static void 
my_remove (struct intr_frame *f) {
	printf("  ### In remove\n");

	const char *filename = (char *)(4+(f->esp));
	struct file *file;

  // if (invalid_ptr (file))
  // {
  // 	//exit
  // 	return;
  // }

  f->eax = filesys_remove (filename);  //returns bool
}

/* 
Opens the file called file. Returns a nonnegative integer handle called a 
"file descriptor" (fd) or -1 if the file could not be opened.

File descriptors numbered 0 and 1 are reserved for the console: fd 0 
(STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. 
The open system call will never return either of these file descriptors, 
which are valid as system call arguments only as explicitly described below.

Each process has an independent set of file descriptors. File descriptors 
are not inherited by child processes.

When a single file is opened more than once, whether by a single process or 
different processes, each open returns a new file descriptor. Different file 
descriptors for a single file are closed independently in separate calls to 
close and they do not share a file position.
*/
static void 
my_open (struct intr_frame *f) {
	printf("  ### In open\n");

	const char *filename = (char *)(4+(f->esp));  //what is this???
	struct thread *cur = thread_current ();

	//open the file
	struct file *cur_file = filesys_open (filename);
	//get the next open file descriptor available, and put the file in it
	int fd = next_fd (cur);
	cur->open_files[fd-2] = cur_file;

	f->eax = fd;		//return int;
}

/*
Returns the size, in bytes, of the file open as fd.
*/
static void 
my_filesize (struct intr_frame *f) {
	printf("  ### In filesize\n");

	int fd = *((int *)(4+(f->esp)));
	struct file *cur_file = get_file (fd);

	off_t size = file_length (cur_file); 

	f->eax = size;    //return value in eax
}

/* 
Reads size bytes from the file open as fd into buffer. Returns the number of
bytes actually read (0 at end of file), or -1 if the file could not be read
(due to a condition other than end of file). fd 0 reads from the keyboard using
input_getc().
*/
static void 
my_read (struct intr_frame *f) {
	printf("  ### In read\n");

	int fd = *((int *)(4+(f->esp)));
	void *buffer = (void *)(8+(f->esp)); 
	unsigned length = *((int *)(12+(f->esp)));

	//off_t file_read (struct file *, void *, off_t);
	//off_t file_read_at (struct file *, void *, off_t size, off_t start);

	//return int;
}

/* 
Writes size bytes from buffer to the open file fd. Returns the number of bytes 
actually written, which may be less than size if some bytes could not be written
Writing past end-of-file would normally extend the file, but file growth is not 
implemented by the basic file system. The expected behavior is to write as many 
bytes as possible up to end-of-file and return the actual number written, or 0 
if no bytes could be written at all.

fd 1 writes to the console. Your code to write to the console should write all 
of buffer in one call to putbuf(), at least as long as size is not bigger than a
few hundred bytes. (It is reasonable to break up larger buffers.) Otherwise, 
lines of text output by different processes may end up interleaved on the 
console, confusing both human readers and our grading scripts.
*/
static void 
my_write (struct intr_frame *f) {
	//printf("  ### In write\n");

	int fd = *((int *)(4+(f->esp)));
	void *buffer = (void *)*(int*)(8+(f->esp));
	unsigned length = *((int *)(12+(f->esp)));

	//printf("FFFFF fd= %d   ", fd);
	//printf("LLLLL length= %d\n", length);

	if (fd == STDOUT_FILENO)
	{
	  putbuf ((char *)buffer, length);
  }
	//printf("THING TO PRINT: %s\n", (char*)buffer);  //WHAT DO WE PRINT?

	//off_t file_write (struct file *, const void *, off_t);
	//off_t file_write_at (struct file *, const void *, off_t size, off_t start);

  f->eax = length;   //return int == bytes actaully written
}

/*
Changes the next byte to be read or written in open file fd to position, 
expressed in bytes from the beginning of the file. (Thus, a position of 0 is the 
file's start.)

A seek past the current end of a file is not an error. A later read obtains 0 
bytes, indicating end of file. A later write extends the file, filling any 
unwritten gap with zeros. (However, in Pintos, files will have a fixed length 
until project 4 is complete, so writes past end of file will return an error.) 
These semantics are implemented in the file system and do not require any 
special effort in system call implementation.
*/
static void 
my_seek (struct intr_frame *f) {
	printf("  ### In seek\n");

	int fd = *((int *)(4+(f->esp))); 
	unsigned position = *((int *)(4+(f->esp)));
}

/*
Returns the position of the next byte to be read or written in open file fd,
expressed in bytes from the beginning of the file. 
*/
static void 
my_tell (struct intr_frame *f) {
	printf("  ### In tell\n");

	int fd = *((int *)(4+(f->esp)));
	get_file (fd);

	// return unsigned;
}

/* 
Closes file descriptor fd. Exiting or terminating a process implicitly closes 
all its open file descriptors, as if by calling this function for each one.
*/
static void 
my_close (int fd) {
	printf("  ### In Close\n");
  
  //GIVEN THE FD, CLOSE THE FILE
	struct file *cur_file = get_file (fd);

	struct thread *cur = thread_current ();
	cur->open_files[fd-2] = NULL;  //IS THIS CORRECT?
	
	file_close (cur_file);
}

/*
Helper method: Given param file descriptor,
function returns the appropriate file
*/
static struct file *
get_file (int fd)
{
  struct thread *cur = thread_current ();

  if(fd < 2 || fd > (129)) {
  	return NULL;
  }
  return cur->open_files[fd-2]; 
}

static int
next_fd (struct thread *cur) {
	int found = 0;
	int index = 0;
	while (!found) {
		if (cur->open_files[index] == NULL) {
			found = 1;
			return index + 2;
		}
		index++;
	}
	return -1;
}
