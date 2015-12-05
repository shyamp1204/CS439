#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
struct inode* traverse_path (struct dir *dir, char* path);
char* traverse_path_name (struct dir *dir, char* path);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  thread_current()->current_working_dir = ROOT_DIR_SECTOR; 

  if (format) 
    do_format ();
  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  bool success = false;

  if(name != NULL && strlen (name) > 0)
  {
    block_sector_t inode_sector = 0;
    struct dir *dir = dir_open_root ();
    success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, is_dir)
                  && dir_add (dir, name, inode_sector));
    if (!success && inode_sector != 0) 
      free_map_release (inode_sector, 1);
    dir_close (dir);
  }

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. 
   KK writing here */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16, ROOT_DIR_SECTOR))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/*creates a directory file 
  alex driving here
*/
bool filesys_mkdir (char *path_name)
{
  return filesys_create (path_name, 0, true);
}

/* change directories */
bool filesys_chdir (const char *path_name)
{
  return true;
}

/*
  given a parent directory and path, returns pointer to the inode
  KK and Wes driving here
*/
struct inode*
traverse_path (struct dir *dir, char* path)
{
  struct inode *inode = NULL;
  char * save_ptr;
  char * token;
  if (path[0] == "/")
    token = strtok_r (path, "/", &save_ptr);

  //look in current dir for the file "name"
  for(token = strtok_r (path, "/", &save_ptr); token != NULL && dir != NULL; token = strtok_r (NULL, "/", &save_ptr)) {
    if(!dir_lookup (dir, token, &inode))
      return NULL;
    //now, *inode is set to the inode associated with the file
    if(!is_dir(inode))
    {
      //if it's a file, at end of path, so file_open and return inode
      file_open (inode);
      return inode;
    }
    dir = dir_open (inode);
  }
  return inode;
}

