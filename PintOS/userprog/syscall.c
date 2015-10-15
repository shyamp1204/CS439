#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

void halt (void);
void exit (int status);
pid_t exec (const char *file); //file is same as cmd_line
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);


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

	* Project 3 and optionally project 4. 
	 
	SYS_MMAP,		 Map a file into memory. 
	SYS_MUNMAP,		 Remove a memory mapping. 

	* Project 4 only. 
	 
	SYS_CHDIR,		 Change the current directory. 
	SYS_MKDIR,		 Create a directory. 
	SYS_READDIR,	 Reads a directory entry. 
	SYS_ISDIR,		 Tests if a fd represents a directory. 
	SYS_INUMBER		 Returns the inode number for a fd. 
};
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
int invalid_ptr (char *ptr) {

}


/*
Terminates Pintos by calling shutdown_power_off() (declared in devices/shutdown.h).
 This should be seldom used, because you lose some information about possible 
 deadlock situations, etc. */
void halt (void) {

}

/*Whenever a user process terminates, because it called exit or for any other 
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
void exit (int status) {

}

/* file is same as cmd_line

Runs the executable whose name is given in cmd_line, passing any given 
arguments, and returns the new process's program id (pid). Must return pid -1,
 which otherwise should not be a valid pid, if the program cannot load or run 
 for any reason. Thus, the parent process cannot return from the exec until it
  knows whether the child process successfully loaded its executable. You must 
  use appropriate synchronization to ensure this. */
pid_t exec (const char *file)  {

} 

/*
 ALOT.....
 */
int wait (pid_t pid)  {

}

/*
Creates a new file called file initially initial_size bytes in size. Returns 
true if successful, false otherwise. Creating a new file does not open it: o
pening the new file is a separate operation which would require a open system 
call. */
bool create (const char *file, unsigned initial_size) {

}

/*
Deletes the file called file. Returns true if successful, false otherwise. 
A file may be removed regardless of whether it is open or closed, and removing
an open file does not close it. */
bool remove (const char *file) {

}

/* */
int open (const char *file) {

}

/* */
int filesize (int fd) {

}

/* 
Reads size bytes from the file open as fd into buffer. Returns the number of
bytes actually read (0 at end of file), or -1 if the file could not be read
(due to a condition other than end of file). fd 0 reads from the keyboard using
input_getc().
*/
int read (int fd, void *buffer, unsigned length) {

}

/* */
int write (int fd, const void *buffer, unsigned length) {

}

/* */
void seek (int fd, unsigned position) {

}

/*
Returns the position of the next byte to be read or written in open file fd,
expressed in bytes from the beginning of the file. */
unsigned tell (int fd) {

}

/* 
Closes file descriptor fd. Exiting or terminating a process implicitly closes 
all its open file descriptors, as if by calling this function for each one.
*/
void close (int fd) {

}


