#include <hash.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/page.h"

struct frame
{
	void* addr;  //PHYSICAL ADDRESS OF FRAME
	void* page;  //POINTER TO THE PAGE WITHIN THIS FRAME
	struct thread* thread;  //THREAD THAT OWNS THIS FRAME
	bool pinned;  //TRUE IS THE FRAME IS "PINNED"
	struct hash_elem hash_elem;  //HASH ELEMENT FOR IMPLEMENTING HASH TABLE FOR FRAME TABLE
}