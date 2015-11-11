#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "filesys/off_t.h"
#include "lib/kernel/list.h"

/* 
Struct that holds information about a page in the supplementary page table. 
*/
struct sup_page
  {
    struct list_elem spage_elem;
    void *v_addr;
    bool page_in_swap;
    struct file *file;
    off_t offset;
    int swap_index;
  };

void spage_init();
struct sup_page* create_sup_page (struct file*, off_t offset, void *addr);
bool add_sup_page (struct sup_page *page);
struct sup_page* get_sup_page (void *addr);
void delete_sup_page (struct sup_page *page);

#endif /* vm/page.h */
