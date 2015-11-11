#include "vm/page.h"
#include "threads/pte.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "vm/swap.h"

static struct list sup_page_list;   //list to keep track of all supplemental pages


//initialize the supplemental page table
void
spage_init()
{
  list_init (&sup_page_list);
}

/* 
Create a supplemental page and gets its values
*/
struct sup_page* create_sup_page (struct file *f, off_t offset, void *addr)
{
  struct sup_page *s_page = malloc (sizeof (struct sup_page));
  if (s_page == NULL)
    PANIC ("Failed to allocate memory");

  s_page->v_addr = addr;
  s_page->page_in_swap = false;
  s_page->file = f;
  s_page->offset = offset;
  s_page->swap_index = -1;

  add_sup_page(s_page);   //DO I NEED THIS HERE?

  return s_page;
}

/* 
Remove the supplemental page table entry from the current thread's
supplemental page table 
*/
void
delete_sup_page (struct sup_page *page)
{
  struct sup_page *temp;
  struct list_elem *temp_elem;
  bool found = false;

  for (temp_elem = list_begin (&sup_page_list); temp_elem != list_end (&sup_page_list) && !found; temp_elem = list_next (temp_elem)) 
  {
    temp = list_entry (temp_elem, struct sup_page, spage_elem);
    if (temp->v_addr == page->v_addr) {
      found = true;
      list_remove (temp_elem);
      //FREE????  free (temp);
    }
  }
}

/* 
add the supplemental page to our data structure 
*/
bool 
add_sup_page (struct sup_page *page)
{
  list_push_back (&sup_page_list, &page->spage_elem);
}

/* 
Find the sup_page in PT which has the address ADDR. Return a null pointer
if the sup_page is not found 
*/
struct sup_page*
get_sup_page (void *addr)
{
  struct sup_page *temp;
  struct list_elem *temp_elem;

  for (temp_elem = list_begin (&sup_page_list); temp_elem != list_end (&sup_page_list); temp_elem = list_next (temp_elem)) 
  {
    temp = list_entry (temp_elem, struct sup_page, spage_elem);
    if (temp->v_addr == addr) {
      return temp;
    }
  }
}

