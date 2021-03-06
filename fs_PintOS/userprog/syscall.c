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
#include "filesys/file.h"
#include "devices/input.h"
#include "threads/synch.h"
#include "lib/string.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "filesys/directory.h"

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
static void exit_status (int e_status);
static void my_chdir (struct intr_frame *f);
static void my_mkdir (struct intr_frame *f);
static void my_readdir (struct intr_frame *f);
static void my_isdir (struct intr_frame *f);
static void my_inumber (struct intr_frame *f);
struct lock filesys_lock;

/*initilizes structs in syscall.c*/
void
syscall_init (void) 
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init (&filesys_lock);
}

/*function to handle the interrupt frame and call correct system call
	Alex drove here
*/
static void
syscall_handler (struct intr_frame *f) 
{

	if (invalid_ptr (f->esp))
	{
		exit_status (-1);
		return;
	}

	// get the syscall type from the stack      
	int syscall_num = *((int *)f->esp);

	// Based on the enumerated_type of the arg call the respective syscall func
	switch (syscall_num)
	{
		case SYS_HALT:
			my_halt ();
			break;
		case SYS_EXIT:
			my_exit (f);
			break;
		case SYS_EXEC:
			my_exec (f);
			break;
		case SYS_WAIT:
			my_wait (f);
			break;
		case SYS_CREATE:
			my_create (f);
			break;
		case SYS_REMOVE:
			my_remove (f);
			break;
		case SYS_OPEN:
			my_open (f);
			break;
		case SYS_FILESIZE:
			my_filesize (f);
			break;
		case SYS_READ:
			my_read (f);
			break;
		case SYS_WRITE:
			my_write (f);
			break;
		case SYS_SEEK:
			my_seek (f);
			break;
		case SYS_TELL:
			my_tell (f);
			break;
		case SYS_CLOSE:
			my_close (*((int *)(4+(f->esp))));
			break;
		case SYS_CHDIR:
			my_chdir (f);
			break;
		case SYS_MKDIR:
			my_mkdir (f);
			break;
		case SYS_READDIR:
			my_readdir (f);
			break;
		case SYS_ISDIR:
			my_isdir (f);
			break;
		case SYS_INUMBER:
			my_inumber (f);
			break;
		default :
			exit_status (-1);  
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
Wes, Alex and KK driving
*/
static int 
invalid_ptr (void *ptr) 
{
	if (ptr == NULL || !is_user_vaddr (ptr) || pagedir_get_page (thread_current ()->pagedir, ptr) == NULL) 
		return 1;
	return 0;
}

/* 
Exits from the thread, printing exit information.
Alex, Wes And Katherine Drove
*/
static void
exit_status (int e_status)
{
	struct thread *cur = thread_current ();

	//if user process terminates, print process' name and exit code
	printf("%s: exit(%d)\n", thread_name (), e_status);

	// close all open files
	int numof_file;
	for (numof_file = 2; numof_file < 129; numof_file++) 
	{
		//GET NEXT OPEN FILE and close it
		struct file *temp_file = cur->open_files[numof_file];
		if (temp_file != NULL) {
			my_close (numof_file);  		//close the file
		}
	}

	//add the status to the tid
	cur->my_info->exit_status = e_status;
	thread_exit ();
}

/*
Terminates Pintos by calling shutdown_power_off() (declared in 
devices/shutdown.h).
This should be seldom used, because you lose some information about possible 
deadlock situations, etc. 
Alex and Wes Drove
*/
static void 
my_halt (void) 
{
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
Alex Drove
*/
static void 
my_exit (struct intr_frame *f) 
{	
	int e_status = *((int *)(4+f->esp));

	if (e_status < -1) {
  		exit_status (-1);
  		return;
 	}

	exit_status (e_status);
	f->eax = e_status;		//return value in eax
}

/* 
file is same as cmd_line

Runs the executable whose name is given in cmd_line, passing any given 
arguments, and returns the new process's program id (pid). Must return pid -1,
which otherwise should not be a valid pid, if the program cannot load or run 
for any reason. 

Thus, the parent process cannot return from the exec until it
knows whether the child process successfully loaded its executable. You must 
use appropriate synchronization to ensure this.
Alex and Katherine Drove
*/
static void 
my_exec (struct intr_frame *f)  
{
	const char *filename = (char *)*(int*)(4+(f->esp));

	//CHECK TO MAKE SURE THE FILEname POINTER IS VALID
	if (invalid_ptr ((void *)filename)) 
	{
  	exit_status (-1);
  	return;
 	}

  lock_acquire (&filesys_lock);
  //adds child to curent threads child list
  tid_t pid = process_execute ((char*)filename);

  if(pid != TID_ERROR) 
  	sema_down (&(thread_current()->exec_sema));

  lock_release (&filesys_lock);
 	(pid == TID_ERROR) ? (f->eax = -1) : (f->eax = pid);		//return pid_t;
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
Alex Drove
*/
static void 
my_wait (struct intr_frame *f)  
{
  pid_t pid = *((int *)(4+(f->esp)));
  f->eax = process_wait (pid);
}

/*
Creates a new file called file initially initial_size bytes in size. Returns 
true if successful, false otherwise. Creating a new file does not open it: 
opening the new file is a separate operation which would require a open system 
call. 
Alex and Wes Drove
*/
static void 
my_create (struct intr_frame *f) 
{
	const char *filename = (char *)*(int*)(4+(f->esp));
	unsigned initial_size = *((int *)(8+(f->esp)));

 	if (invalid_ptr((void *) filename) || *filename == NULL || strlen (filename) <= 0)
 	{  
		exit_status (-1);
    return;
 	}

 	lock_acquire (&filesys_lock);
	f->eax = filesys_create (filename, initial_size, false);   //returns boolean
	lock_release (&filesys_lock);
}

/*
Deletes the file called file. Returns true if successful, false otherwise. 
A file may be removed regardless of whether it is open or closed, and 
removing an open file does not close it.
Alex and Katherine Drove 
*/
static void 
my_remove (struct intr_frame *f) 
{
	//UPDATE FOR DIRECTORIES
	const char *filename = (char *)*(int*)(4+(f->esp));

  if (invalid_ptr ((void *)filename)) 
  {
  	exit_status (-1);
  	return;
  }

	lock_acquire (&filesys_lock);
  f->eax = filesys_remove (filename);  //returns bool
  lock_release (&filesys_lock);
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
Alex Drove
*/
static void 
my_open (struct intr_frame *f) 
{
	//MUST ALSO OPEN DIRECTORIES
	const char *filename = (char *)*(int*)(4+(f->esp));
	struct thread *cur = thread_current ();

	if (invalid_ptr ((void *) filename)) 
	{
  		exit_status (-1);
  		return;
  }

	lock_acquire (&filesys_lock);
	struct file *cur_file = filesys_open (filename);
	lock_release (&filesys_lock);

	if (cur_file != NULL) 
	{
		//get the next open file descriptor available, and put the file in it
		int fd = next_fd (cur);
		cur->open_files[fd-2] = cur_file;
		f->eax = fd;		//return int;
	}
	else 
	{
		f->eax = -1;  //couldn't open the file, return -1
	}
}

/*
Returns the size, in bytes, of the file open as fd.
Alex Drove*/
static void 
my_filesize (struct intr_frame *f) {
	int fd = *((int *)(4+(f->esp)));
	struct file *cur_file = get_file (fd);

	lock_acquire (&filesys_lock);
	off_t size = file_length (cur_file);
	lock_release (&filesys_lock);

	f->eax = size;		//size = length of the file
}

/* 
Reads size bytes from the file open as fd into buffer. Returns the number of
bytes actually read (0 at end of file), or -1 if the file could not be read
(due to a condition other than end of file). fd 0 reads from the keyboard using
input_getc().
Alex and Katherine Drove
*/
static void 
my_read (struct intr_frame *f) {
	int fd = *((int *)(4+(f->esp)));
	void *buffer = (void *)*(int*)(8+(f->esp)); 
	int length = *((int *)(12+(f->esp)));

	if (invalid_ptr (buffer)) 
	{
	 	exit_status (-1);
	 	return;
	 }

	if (fd == STDIN_FILENO) 
	{
		lock_acquire (&filesys_lock);
		char *buffer = (char *) buffer;
		int i;
		for (i = 0; i < length; i++) 
		{
			buffer[i] = input_getc();		//read each char from keyboard
		}
		lock_release (&filesys_lock);
		f->eax = length;			//return bytes read
	}
	else 
	{
		struct thread *cur = thread_current ();
		struct file *cur_file = get_file (fd);

	  if(fd < 2 || fd > 129 || cur->open_files[fd-2] == NULL) 
	  {
	  	f->eax = -1;
	  }
	  else 
	  {
	  	lock_acquire (&filesys_lock);
	  	f->eax = file_read (cur_file, buffer, length);  //return bytes read
	  	lock_release (&filesys_lock);
	  }
	} 
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
Alex and Wes Drove
*/
static void 
my_write (struct intr_frame *f) 
{
	int fd = *((int *)(4+(f->esp)));
	void *buffer = (void *)*(int*)(8+(f->esp));
	unsigned length = *((int *)(12+(f->esp)));

	if (invalid_ptr (buffer)) 
	{
  	exit_status (-1);
  	return;
  }

	if (fd == STDOUT_FILENO) 
	{
		lock_acquire (&filesys_lock);
		putbuf ((char *)buffer, length);
		lock_release (&filesys_lock);
		f->eax = length;		//return bytes written to console
  }
  else 
  {
		struct thread *cur = thread_current ();

	  if(fd < 2 || fd > 129 || cur->open_files[fd-2] == NULL) 
	  {
	  	exit_status (-1);
	  	f->eax = -1;			//return -1 if could not write
	  }
	  else 
	  {
	  	lock_acquire (&filesys_lock);
	  	struct file *cur_file = get_file (fd);

	  	if (cur->exec_file != cur_file)
				f->eax = file_write (cur_file, buffer, length);  //return bytes written

	  	lock_release (&filesys_lock);
	  }
	}
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
Alex Drove
*/
static void 
my_seek (struct intr_frame *f) 
{
	int fd = *((int *)(4+(f->esp))); 
	int position = *((int *)(8+(f->esp)));  //unsigned

	lock_acquire (&filesys_lock);
	struct file *cur_file = get_file(fd);
	off_t size = file_length (cur_file);
	if (position < 0) 
		return;
	// else if (position > size)
	// 	position = size;

	file_seek (cur_file, position);
	lock_release (&filesys_lock);
}

/*
Returns the position of the next byte to be read or written in open file fd,
expressed in bytes from the beginning of the file. 
Alex Drove
*/
static void 
my_tell (struct intr_frame *f) 
{
	int fd = *((int *)(4+(f->esp)));
	struct thread *cur = thread_current ();

  if (fd < 2 || fd > 129 || cur->open_files[fd-2] == NULL)
  {
  	f->eax = -1;			//return -1 if could not read
  }
  else 
  {
		struct file *cur_file = get_file(fd);
		lock_acquire (&filesys_lock);
		f->eax = file_tell (cur_file);		// return position (unsigned)
		lock_release (&filesys_lock);
	}
}

/* 
Closes file descriptor fd. Exiting or terminating a process implicitly closes 
all its open file descriptors, as if by calling this function for each one.
Alex Drove
*/
static void 
my_close (int fd) 
{  
  //GIVEN THE FD, CLOSE THE FILE
	struct file *cur_file = get_file (fd);

	struct thread *cur = thread_current ();
	cur->open_files[fd-2] = NULL;
	
	lock_acquire (&filesys_lock);
	file_close (cur_file);
	lock_release (&filesys_lock);
}

/*
Helper method: Given param file descriptor,
function returns the appropriate file
Alex Drove
*/
static struct file *
get_file (int fd)
{
  struct thread *cur = thread_current ();

  if(fd < 2 || fd > (129) || cur->open_files[fd-2] == NULL) 
  {
  	exit_status (-1);
  }
  return cur->open_files[fd-2];
}

/*
Return the next file discriptor from a threads open thread list
Alex drove
*/
static int
next_fd (struct thread *cur) 
{
	int found = 0;
	int index = 0;
	while (!found) 
	{
		if (cur->open_files[index] == NULL) 
		{
			found = 1;
			return index + 2;
		}
		index++;
	}
  exit_status (-1);
	return -1;
}

/*
function for outside files to call exit_status.  KK drove
*/
void
exit_status_ext (int e_status) {
	exit_status(e_status);
}

/*
Changes the current working directory of the process to dir, which may be 
relative or absolute. Returns true if successful, false on failure. 
Alex drove
*/ 
static void 
my_chdir (struct intr_frame *f) 
{
	char *dir_name = (char *)*(int*)(8+(f->esp));
		
	lock_acquire (&filesys_lock);
	f->eax = filesys_chdir (dir_name); //false;
	lock_release (&filesys_lock);
}

/*
  Creates the directory named dir, which may be relative or absolute. 
  Returns true if successful, false on failure. 
  Fails if dir already exists or if any directory name in dir, besides 
  the last, does not already exist. 
  That is, mkdir("/a/b/c") succeeds only if "/a/b" already exists and "/a/b/c" 
  does not. 
	Alex drove
*/
static void 
my_mkdir (struct intr_frame *f) 
{
	char *dir_name = (char *)*(int*)(4+(f->esp));
		
	if (invalid_ptr (dir_name)) 
	{
  	exit_status (-1);
  	return;
  }

	lock_acquire (&filesys_lock);
  f->eax = filesys_mkdir (dir_name);
	lock_release (&filesys_lock);
}

/*
Reads a directory entry from file descriptor fd, which must represent a directory. 
If successful, stores the null-terminated file name in name, which must have room 
for READDIR_MAX_LEN + 1 bytes, and returns true. If no entries are left in the directory, returns false.
"." and ".." should not be returned by readdir.
If the directory changes while it is open, then it is acceptable for some 
entries not to be read at all or to be read multiple times. Otherwise, each 
directory entry should be read once, in any order.
READDIR_MAX_LEN is defined in "lib/user/syscall.h". If your file system supports 
longer file names than the basic file system, you should increase this value from the default of 14.
Alex drove
*/
static void 
my_readdir (struct intr_frame *f) 
{
	int fd = *((int *)(4+(f->esp)));
	char *dir_name = (char *)*(int*)(8+(f->esp));

	if (invalid_ptr (dir_name)) 
	{
  	exit_status (-1);
  	return;
  }

  struct file *cur_file =  get_file (fd);

  struct dir *mydir = malloc (sizeof *mydir);
  // mydir->inode = get_inode_from_file (cur_file);
  // mydir->pos = get_pos_from_file (cur_file);

	lock_acquire (&filesys_lock);
  f->eax = dir_readdir (mydir, dir_name);	// return bool;
  lock_release (&filesys_lock);
  free (mydir);
}

/*
Returns true if fd represents a directory, false if it represents an ordinary file. 
Alex drove
*/
static void 
my_isdir (struct intr_frame *f) 
{
	int fd = *((int *)(4+(f->esp)));
	struct file *cur_file = get_file (fd);

	f->eax = true;	//return bool;	//bool dir_lookup (const struct dir *, const char *name, struct inode **);
	//struct dir* dir_getdir (const char *path_name)
}

/*
Returns the inode number of the inode associated with fd, which may represent an ordinary file or a directory.
An inode number persistently identifies a file or directory. It is unique during the file's existence. In Pintos, the sector number of the inode is suitable for use as an inode number.
Alex drove
*/
static void 
my_inumber (struct intr_frame *f)
{
	int fd = *((int *)(4+(f->esp)));
	struct file *cur_file = get_file (fd);

	lock_acquire (&filesys_lock);
	f->eax = get_sector_from_file (cur_file);		// return int;
	lock_release (&filesys_lock);
}
