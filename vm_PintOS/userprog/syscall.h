#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void exit_status_ext (int e_status);
void filesys_lock_aquire (void);
void filesys_lock_release (void);

#endif /* userprog/syscall.h */
