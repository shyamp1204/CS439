#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "page.h"

static const size_t NUM_SECTORS_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;
struct block* block_device;
//bitmap = 0 means it is FREE, 1 means it is IN USE
struct bitmap* swap_bitmap;
struct lock swap_lock;
bool is_initialized = false;

/*
initialize the swap table
Katherine driving.
*/
void
swap_init (void)
{
	//initialize block device
	block_device = block_get_role (BLOCK_SWAP);
	if (block_device == NULL)
	{
		PANIC("Called before the OS initialized block devices. Can't get block.");
	}
	//bitmap size = number of pages per block
	size_t bitmap_size = block_size (block_device) / NUM_SECTORS_PER_PAGE;
	swap_bitmap = bitmap_create (bitmap_size);
	if (swap_bitmap == NULL)
	{
		PANIC ("Memory allocation failed for bitmap swap table.");
	}
	lock_init (&swap_lock);
}

void 
change_page_location (void *addr, location t, int swap_index) {
	struct sup_page* spage = get_sup_page (addr);
	spage->page_location = t;
	spage->swap_index = swap_index;
}


// load page from swap into main memory; return pa in main memory
void
load_swap (void* uaddr, int swap_index)
{
	//make sure swap_index is not null and is a valid index
	lock_acquire(&swap_lock);
	bool valid_bitmap = bitmap_test(swap_bitmap, swap_index);
	if(!valid_bitmap)
		PANIC("Swap slot is invalid.");
	//then read the slot into memory
	block_sector_t sector_base = swap_index * NUM_SECTORS_PER_PAGE;
	unsigned i;
	for(i = 0; i < NUM_SECTORS_PER_PAGE; i++)
	{
		block_read(block_device, sector_base + i, (BLOCK_SECTOR_SIZE * i) + uaddr);
	}

	//set slot to FREE!
	bitmap_set(swap_bitmap, swap_index, false);
	location new_frame_location = IN_MEMORY;
	change_page_location(uaddr, new_frame_location, -1);
	lock_release(&swap_lock);
}

// send page from main memory to swap and store it in swap
int
store_swap (void *uaddr)
{
	if (!is_initialized) {
		  swap_init ();
		  is_initialized = true;
	}
	//scan bitmap for an open slot
	lock_acquire (&swap_lock);

	int swap_index = bitmap_scan_and_flip (swap_bitmap, 0, 1, false);

	//then write the page in memory to this slot in swap
	block_sector_t sector_base = swap_index * NUM_SECTORS_PER_PAGE;
	unsigned i;
	for (i = 0; i < NUM_SECTORS_PER_PAGE; i++)
	{
		block_write(block_device, sector_base + i, (BLOCK_SECTOR_SIZE * i) + uaddr);
	}

	location new_frame_location = IN_SWAP;
	change_page_location (uaddr, new_frame_location, swap_index);
	lock_acquire (&swap_lock);

	return swap_index;
}

