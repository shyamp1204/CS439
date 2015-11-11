#include <hash.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/page.h"

struct frame
{
	void* page;  								//POINTER TO THE PAGE MAPPED TO THIS FRAME
	struct thread* thread;			//THREAD THAT OWNS THIS FRAME
	struct hash_elem hash;		 //HASH ELEMENT FOR IMPLEMENTING HASH TABLE to store this frame in the frame table
	struct list_elem fifo_elem;
	// bool pinned;  						//TRUE IS THE FRAME IS "PINNED" -- may not need this!
};

void
frame_init();

static unsigned
hash_func (struct hash_elem *e, void *aux);

static bool
less_func (struct hash_elem *a, struct hash_elem *b, void *aux);

//get a free frame by calling palloc_get_page and allocate a frame; if no frame available, evict a frame and use that
void*
get_frame(enum palloc_flags);

//remove a frame from the frame table and free it
void
free_frame(void*);		//page_vaddr, uint32_t* pagedir

//choose a frame to evict and clear it from the page directory of its owner thread
void*
evict_frame (void);

//maps the given frame to a page from the user pool
void
frame_map(void* , void* , bool);

//unmaps the given frame
void
frame_unmap(void*);