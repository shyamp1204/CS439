#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define NUM_DIRECT_PTR 123

#define NUM_PTR_PER_BLOCK 128

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE (512 = 2^9) bytes long. 
   ptr = 8 or 2^3*/
struct inode_disk
{
  block_sector_t start;               /* First data sector. */ // don't need this anymore
  off_t length;                       /* File size in bytes. */
  unsigned magic;                     /* Magic number. */
  
  block_sector_t direct_block_sectors[NUM_DIRECT_PTR];   // Direct Pointers Array  //might need to change that number
  block_sector_t indirect_block_sector;                       // First lvl Indirect pointers
  block_sector_t doubly_indirect_block_sector;                //Second lvl indirect pointers
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
{
  struct lock data_lock;
  struct list_elem elem;              /* Element in inode list. */
  block_sector_t sector;              /* Sector number of disk location. */
  int open_cnt;                       /* Number of openers. */
  bool removed;                       /* True if deleted, false otherwise. */
  int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  struct inode_disk data;             /* Inode content. */
};

struct indirect_block
{
   block_sector_t direct_block_sectors[NUM_PTR_PER_BLOCK];
};

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
    return inode->data.start + pos / BLOCK_SECTOR_SIZE;
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}


int
create_indirect(size_t remaining_sectors, struct indirect_block *first_lvl_id)
{
  static char zeros[BLOCK_SECTOR_SIZE];

  int index;
  for(index = 0; index < NUM_PTR_PER_BLOCK && index < remaining_sectors; index++)
  {
    //Allocate a sector
    free_map_allocate (1, &(first_lvl_id[index]));

    //Fill sector with Zeros
    block_write (fs_device, &(first_lvl_id[index]), zeros);

    remaining_sectors--;
  } 
  if (index < NUM_PTR_PER_BLOCK)  // might not need this just a sneaky suspicion 
  {
    ;//did not use all of the 1st level indirect block
  }

  return index;
}



/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool 
inode_create (block_sector_t sector, off_t length){
  //init lock       data_lock
  //lock aquire
  //init inode disk
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);

  if (disk_inode != NULL)
  {
    static char zeros[BLOCK_SECTOR_SIZE];

    //initalize the inode on heap
    size_t sectors = bytes_to_sectors (length);
    size_t remaining_sectors = sectors;

    disk_inode->length = length;
    disk_inode->magic = INODE_MAGIC;

    //alocate direct pointers
    if(remaining_sectors > 0)
    {
      int index;
      for(index = 0; index < NUM_DIRECT_PTR && index < remaining_sectors; index++){
        //Allocate a sector
        free_map_allocate (1, &disk_inode->direct_block_sectors[index]);

        //Fill sector with Zeros
        block_write (fs_device, &disk_inode->direct_block_sectors[index], zeros);

        remaining_sectors--;
      }
    }

    //check remaining length
    //write to first lvl indirect
    if(remaining_sectors > 0)
    {
      struct indirect_block *first_lvl_id = NULL;  
      /* If this assertion fails, the inode structure is not exactly
      one sector in size, and you should fix that. */
      ASSERT (sizeof *first_lvl_id == BLOCK_SECTOR_SIZE);

      first_lvl_id = calloc (1, sizeof *first_lvl_id);

      //allocate the indirect block
      free_map_allocate (1, &disk_inode->indirect_block_sector);

      remaining_sectors -= create_indirect(remaining_sectors, first_lvl_id);

      //write to disk the inode we made on heap
      block_write (fs_device, disk_inode->indirect_block_sector, first_lvl_id);

      free(first_lvl_id);                             
    } 
    else 
      disk_inode->indirect_block_sector == NULL;

    
    //check remaining length
    //write to second lvl indirect
    if(remaining_sectors > 0)
    {
      struct indirect_block *second_lvl_id = NULL;
      /* If this assertion fails, the inode structure is not exactly
      one sector in size, and you should fix that. */
      ASSERT (sizeof *second_lvl_id == BLOCK_SECTOR_SIZE);
      second_lvl_id = calloc (1, sizeof *second_lvl_id);

      //Allocate a sector
      free_map_allocate (1, &disk_inode->doubly_indirect_block_sector);
     
      int index;
      for(index = 0; index < NUM_PTR_PER_BLOCK && remaining_sectors > 0; index++)
      {
        struct indirect_block *first_lvl_id = NULL;  
        /* If this assertion fails, the inode structure is not exactly
        one sector in size, and you should fix that. */
        ASSERT (sizeof *first_lvl_id == BLOCK_SECTOR_SIZE);

        first_lvl_id = calloc (1, sizeof *first_lvl_id);

        //Allocate a sector
        free_map_allocate (1, &(second_lvl_id[index]));

        remaining_sectors -= create_indirect(remaining_sectors, first_lvl_id);

        //write to disk the inode we made on heap
        block_write (fs_device, &(second_lvl_id[index]), first_lvl_id);

        free(first_lvl_id);
      } 

      //write to disk the inode we made on heap
      block_write (fs_device, second_lvl_id, &disk_inode->indirect_block_sector);

      free(second_lvl_id);
    } 
    else 
      disk_inode->doubly_indirect_block_sector == NULL;


    //write to disk the inode we made on heap
    block_write (fs_device, sector, disk_inode);
    success = true; 
  
    free (disk_inode);
  }
  //lock release

  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
