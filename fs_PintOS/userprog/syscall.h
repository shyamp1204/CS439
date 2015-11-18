#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void exit_status_ext (int e_status);
bool chdir (const char *dir);
bool mkdir (const char *dir);
bool readdir (int fd, char *name);
bool isdir (int fd);
int inumber (int fd);

#endif /* userprog/syscall.h */
