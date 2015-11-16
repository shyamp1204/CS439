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

static struct lock frame_lock;	// lock for frame table
static struct list fifo_list;		//list to keep track of FIFO order

//initialize the frame table
//Alex and Katherine Drove
void
frame_init(void)
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
Alex Wes and Katherine Drove
*/
void*
get_frame (enum palloc_flags flags)
{
	lock_acquire (&frame_lock);
	//this "addr" variable will be the return value at the end!
	void* addr = palloc_get_page (flags);
	struct frame *myframe;

	if (addr == NULL) 
	{
		//evict a frame and use this new frame (called evict function)
		evict_frame ();
		//should now be able to properly palloc since a page was evicted
		addr = palloc_get_page (flags);
	}

	//We know here there is an available page bc of either empty or evicted spot
	if(addr != NULL) 
	{
		//successful memory allocation!  ADD IT TO OUR DATA STRUCTURE
		myframe = (struct frame*)malloc (sizeof (struct frame));
		myframe->page = addr;
		myframe->thread = thread_current ();

		list_push_back (&fifo_list, &(myframe->fifo_elem));
	}
	lock_release (&frame_lock);
	return addr;
}

/*
choose a frame to evict (using replacement algorithm),
then clear it from the page directory of its "owner" thread.

Alex KK and Wes Drove
*/
void
evict_frame (void)
{
	struct list_elem *temp_elem = list_pop_front (&fifo_list);
	//list entry to get the frame struct from the list_elem
	struct frame *f = list_entry (temp_elem, struct frame, fifo_elem);
	//put into swap
	store_swap(f->page);
  //now put in swap or free
  palloc_free_page (f->page);
}


/* 
remove given frame (addr points to this frame) from the frame table and 
free its resources
Alex and Katherine Drove
*/
void
free_frame (void* frame_addr)
{
	bool found = false;
	struct list_elem* temp_elem;
	struct frame *f;

	//remove (set to empty) the frame from our data structures
	//Loop over the fifo list and remove the elem from the list
	for (temp_elem = list_begin (&fifo_list); temp_elem != list_end (&fifo_list) 
															&& !found; temp_elem = list_next (temp_elem)) 
  {
    f = list_entry (temp_elem, struct frame, fifo_elem);
    if (f->page == frame_addr)
    {
      found = true;
      list_remove (temp_elem);
    }
  }
	palloc_free_page (frame_addr); 
}
