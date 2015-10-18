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


static void syscall_handler (struct intr_frame *);
static int invalid_ptr (void *ptr);
static void my_halt (void);
static void my_exit (struct intr_frame *f);
static pid_t my_exec (struct intr_frame *f); //file is same as cmd_line
static int my_wait (struct intr_frame *f);
static bool my_create (struct intr_frame *f);
static bool my_remove (struct intr_frame *f);
static int my_open (struct intr_frame *f);
static int my_filesize (struct intr_frame *f);
static int my_read (struct intr_frame *f);
static int my_write (struct intr_frame *f);
static void my_seek (struct intr_frame *f);
static unsigned my_tell (struct intr_frame *f);
static void my_close (struct intr_frame *f);


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
  printf("### SYSCALL: %d  ", syscall_num);

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
		printf("### Calling Write");
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
		my_close (f);
		break;
	default :
		printf ("Invalid system call! #%d\n", syscall_num);
		thread_exit();   //Exit???  
		break;
    }
}

/*
enum {
	SYS_HALT,		 Halt the operating system. 
	SYS_EXIT,		 Terminate this process. 
	SYS_EXEC,		 Start another process. 
	SYS_WAIT,		 Wait for a child process to die. 
	SYS_CREATE,		 Create a file. 
	SYS_REMOVE,		 Delete a file. 
	SYS_OPEN,		 Open a file. 
	SYS_FILESIZE,		 Obtain a file's size. 
	SYS_READ,		 Read from a file. 
	SYS_WRITE,		 Write to a file. 
	SYS_SEEK,		 Change position in a file. 
	SYS_TELL,		 Report current position in a file. 
	SYS_CLOSE,		 Close a file. 
*/

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

  	int e_status;
  	if (invalid_ptr (4+(f->esp)))
    	e_status = -1;
  	else 
		e_status = *((int *)(4+(f->esp)));

	printf("%s: exit(%d)\n", thread_name (), e_status);

  	// close all open files
  	struct thread *t = thread_current ();
 		struct list_elem *temp_file;

  	// Need a lock ?
  	while (!list_empty (&t->open_files_list))
    {
    	printf("+++ list of FILES is not empty");
      //temp_file = list_pop_front (&t->open_files_list);
      //struct list_elem *file_elem = list_entry (temp_file, struct list_elem, elem);
      //free (file_elem);
    }
  	// Need to release the lock?

  	//add the status to the tid
  	t->exit_status = e_status;
		f->eax = e_status;    //return value in eax
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
static pid_t 
my_exec (struct intr_frame *f)  {
	printf("  ### In exec\n");

	const char *file = *((int *)(4+(f->esp)));

	return 0;
} 

/*
 ALOT.....
 */
static int 
my_wait (struct intr_frame *f)  {
	printf("  ### In wait\n");

	pid_t pid = *((int *)(4+(f->esp)));

	return 0;
}

/*
Creates a new file called file initially initial_size bytes in size. Returns 
true if successful, false otherwise. Creating a new file does not open it: o
pening the new file is a separate operation which would require a open system 
call. */
static bool 
my_create (struct intr_frame *f) {
	printf("  ### In create\n");

	const char *file = *((int *)(4+(f->esp)));
	unsigned initial_size = *((int *)(8+(f->esp)));

	return false;
}

/*
Deletes the file called file. Returns true if successful, false otherwise. 
A file may be removed regardless of whether it is open or closed, and 
removing an open file does not close it. */
static bool 
my_remove (struct intr_frame *f) {
	printf("  ### In remove\n");

	const char *file = *((int *)(4+(f->esp)));

	return false;
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
static int 
my_open (struct intr_frame *f) {
	printf("  ### In open\n");

	const char *file = *((int *)(4+(f->esp)));

	return 0;
}

/*
Returns the size, in bytes, of the file open as fd.
*/
static int 
my_filesize (struct intr_frame *f) {
	printf("  ### In filesize\n");

	int fd = *((int *)(4+(f->esp)));

	return 0;
}

/* 
Reads size bytes from the file open as fd into buffer. Returns the number of
bytes actually read (0 at end of file), or -1 if the file could not be read
(due to a condition other than end of file). fd 0 reads from the keyboard using
input_getc().
*/
static int 
my_read (struct intr_frame *f) {
	printf("  ### In read\n");

	int fd = *((int *)(4+(f->esp)));
	void *buffer = *((int *)(8+(f->esp))); 
	unsigned length = *((int *)(12+(f->esp)));

	return 0;
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
static int 
my_write (struct intr_frame *f) {
	printf("  ### In write\n");

	int fd = *((int *)(4+(f->esp)));
	const void *buffer = *((int *)(8+(f->esp)));
	unsigned length = *((int *)(12+(f->esp)));

	printf("THING TO PRINT: %s\n", (char*)buffer);

	return 0;
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
static unsigned 
my_tell (struct intr_frame *f) {
	printf("  ### In tell\n");

	int fd = *((int *)(4+(f->esp)));

	return 0;
}


/* 
Closes file descriptor fd. Exiting or terminating a process implicitly closes 
all its open file descriptors, as if by calling this function for each one.
*/
static void 
my_close (struct intr_frame *f) {
	printf("  ### In Close\n");

	int fd = *((int *)(4+(f->esp)));
}


