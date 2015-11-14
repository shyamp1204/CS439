#include "vm/page.h"
#include "threads/pte.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "threads/thread.h"

/* 
Create a supplemental page and gets its values
*/
struct sup_page* create_sup_page (struct file *f, off_t ofs, uint8_t *addr, uint32_t r_bytes, uint32_t z_bytes, bool write)
{
  struct sup_page *s_page = malloc (sizeof (struct sup_page));
  if (s_page == NULL)
    PANIC ("Failed to allocate memory");

  s_page->page_in_swap = false;
  s_page->swap_index = -1;

  s_page->file = f;
  s_page->offset = ofs;
  s_page->v_addr = addr;
  s_page->read_bytes = r_bytes;
  s_page->zero_bytes = z_bytes;
  s_page->writable = write;
  s_page->frame_location = 0;

  add_sup_page (s_page); 
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
  struct list sup_page_list = thread_current()->sup_page_table_list;

  for (temp_elem = list_begin (&sup_page_list); temp_elem != list_end (&sup_page_list) && !found; temp_elem = list_next (temp_elem)) 
  {
    temp = list_entry (temp_elem, struct sup_page, spage_elem);
    if (temp->v_addr == page->v_addr) {
      found = true;
      list_remove (temp_elem);
      free (temp);
    }
  }
}

/* 
add the supplemental page to our data structure 
*/
void 
add_sup_page (struct sup_page *page)
{
  list_push_back (&(thread_current ()->sup_page_table_list), &page->spage_elem);
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
  struct list sup_page_list = thread_current ()->sup_page_table_list;

  for (temp_elem = list_begin (&sup_page_list); temp_elem != list_end (&sup_page_list); temp_elem = list_next (temp_elem)) 
  {
    temp = list_entry (temp_elem, struct sup_page, spage_elem);
    if (temp->v_addr == addr) {
      return temp;
    }
  }
  return NULL;
}

/* 
Remove the supplemental page table entry from the current thread's
supplemental page table 
*/
void
destroy_sup_page_table ()
{
  struct sup_page *temp;
  struct list_elem *temp_elem;
  bool found = false;
  struct list sup_page_list = thread_current ()->sup_page_table_list;

  for (temp_elem = list_begin (&sup_page_list); temp_elem != list_end (&sup_page_list) && !found; temp_elem = list_next (temp_elem)) 
  {
    temp = list_entry (temp_elem, struct sup_page, spage_elem);
    list_remove (temp_elem);
    free (temp);
  }
}
