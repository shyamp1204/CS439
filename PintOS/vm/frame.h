#include <hash.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/page.h"

struct frame
{
	void* page;  								//POINTER TO THE PAGE MAPPED TO THIS FRAME
	struct thread* thread;			//THREAD THAT OWNS THIS FRAME
	struct list_elem fifo_elem;
};

void frame_init(void);

//get a free frame by calling palloc_get_page and allocate a frame; if no frame available, evict a frame and use that
void* get_frame(enum palloc_flags);

//remove a frame from the frame table and free it
void free_frame(void*);		//page_vaddr, uint32_t* pagedir

//choose a frame to evict and clear it from the page directory of its owner thread
void evict_frame (void);
