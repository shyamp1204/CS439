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
//THERE MIGHT BE MORE, BUT THESE ARE THE ONES I AM GUESSING WE WILL NEED TO INCLUDE!


static unsigned hash_func(const struct hash_elem *e, void *aux);
static bool less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);

// hash table of frames
static struct hash hash_frames;

// lock for frame table
static struct lock frame_lock;

//initialize the frame table
void
frame_init()
{
	lock_init(&frame_lock);
	hash_init(&hash_frames, hash_func, less_func, NULL);
}

/*get a free frame (call palloc_get_page) and allocate this frame;
if no frame available, evict a frame and use that one! */
void*
get_frame(enum palloc_flags flags)
{
	//this "addr" variable will be the return value at the end!
	void* addr = palloc_get_page(flags);
	//check if palloc was successful or not
	if(addr != NULL) {
		//successful memory allocation!
	}
	else {
		//evict a frame and use this new frame (called evict function)
		addr = evict_frame();
	}

	return addr;
}


//remove given frame (addr points to this frame) from the frame table and free its resources
void
free_frame(void* frame_addr)
{

}


/*choose a frame to evict (using replacement algorithm),
then clear it from the page directory of its "owner" thread*/
void*
evict_frame(void)
{

}

//maps the given frame to a page from the user pool
void
frame_map(void* frame_addr, void* page_addr, bool writable)
{

}

//unmaps the given frame
void
frame_unmap(void* frame_addr)
{

}


// hash_hash_func function -- hash function for the hash table
static unsigned
hash_func(const struct hash_elem *e, void *aux)
{
	const struct frame *f = hash_entry(e, struct frame, hash_elem);
	return hash_int((unsigned) f->addr);
}


// hash_less_func function -- comparison function for the hash function
static bool
less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	const struct frame *frame_one = hash_entry(a, struct frame, hash_elem);
	const struct frame *frame_two = hash_entry(b, struct frame, hash_elem);
	return frame_one->addr < frame_two->addr;
}
