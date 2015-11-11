#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include <stdio.h>
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"

static struct hash hash_frames;	// hash table of frames
static struct lock frame_lock;	// lock for frame table
static struct list fifo_list;		//list to keep track of FIFO order

//initialize the frame table
void
frame_init()
{
	lock_init (&frame_lock);
	list_init (&fifo_list);
}

/*
get a free frame (call palloc_get_page) and allocate this frame;
if no frame available, evict a frame and use that one! 

    PAL_ASSERT = 001,            Panic on failure. 
    PAL_ZERO = 002,              Zero page contents. 
    PAL_USER = 004               User page. 
*/
void*
get_frame (enum palloc_flags flags)
{
	//this "addr" variable will be the return value at the end!
	void* addr = palloc_get_page (flags);
	struct frame *myframe;

	if (addr == NULL) 
	{
		//evict a frame and use this new frame (called evict function)
		addr = evict_frame ();
		addr = palloc_get_page (flags);
	}

	//We know here that there is an available page bc of either empty or evicted spot
	if(addr != NULL) 
	{
		//successful memory allocation!  ADD IT TO OUR DATA STRUCTURE
		myframe = (struct frame*)malloc (sizeof (struct frame));
		myframe->page = addr;
		myframe->thread = thread_current ();

		list_push_back (&fifo_list, &(myframe->fifo_elem));
	}
	return addr;
}

/*
choose a frame to evict (using replacement algorithm),
then clear it from the page directory of its "owner" thread.

WHERE DO WE PUT THE EVICTED PAGE?  IN SWAP OR JUST FORGET ABOUT IT?
Alex KK and Wes Drove*/
void*
evict_frame (void)
{
	struct list_elem *temp_elem = list_pop_front (&fifo_list);
	//list entry to get the frame struct from the list_elem
	struct frame *f = list_entry (temp_elem, struct frame, fifo_elem);
	//now put in swap or free
	palloc_free_page (f->page);
	// panic ??
}

//remove given frame (addr points to this frame) from the frame table and free its resources
void
free_frame (void* frame_addr)
{
	bool found = false;
	struct list_elem* temp_elem;
	struct frame *f;

	//remove (set to empty) the frame from our data structures
	//Loop over the fifo list and remove the elem from the list
	for (temp_elem = list_begin (&fifo_list); temp_elem != list_end (&fifo_list) && !found; temp_elem = list_next (temp_elem)) 
  {
    f = list_entry (temp_elem, struct frame, fifo_elem);
    if (f->page == frame_addr) {
      found = true;
      list_remove (temp_elem);
    }
  }
	palloc_free_page (frame_addr); 
}

//maps the given frame to a page from the user pool
void
frame_map (void* frame_addr, void* page_addr, bool writable)
{

}


//unmaps the given frame
void
frame_unmap (void* frame_addr)
{

}

//hash_less_func function -- comparison function for the hash function
static bool
less_func (struct hash_elem *a, struct hash_elem *b, void *aux)
{
	const struct frame *frame_one = hash_entry (a, struct frame, hash);
	const struct frame *frame_two = hash_entry (b, struct frame, hash);
	return frame_one->page < frame_two->page;
}

/*
Hash function used from online resources
http://burtleburtle.net/bob/hash/integer.html
*/
static unsigned 
hash_func (struct hash_elem *e, void *aux)
{
	struct frame *f = hash_entry (e, struct frame, hash);
	uint32_t a = (uint32_t)f->page;

  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return a;
}
