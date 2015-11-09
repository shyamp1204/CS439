#include "vm/frame.h"
#include <stdio.h>


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
