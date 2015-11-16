#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "filesys/off_t.h"
#include "lib/kernel/list.h"

typedef enum
  {
    IN_MEMORY,     /* Page is located in memory = 0 */
    IN_SWAP,       /* Page is located in a swap slot = 1 */
    IN_DISK,       /* Page is located on disk = 2 */
    ALL_ZERO        /* Page is an all-zero page = 3 */
  } location;

/* 
Struct that holds information about a page in the supplementary page table. 
*/
struct sup_page
  {
    struct list_elem spage_elem;
    bool page_in_swap;
    int swap_index;

    void *v_addr;
    struct file *file;
    off_t offset;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    location page_location;   //0=memory, 1=swap, 2=disk
  };

// void spage_init(void);
struct sup_page* create_sup_page (struct file *f, off_t ofs, uint8_t *upage, uint32_t r_bytes, uint32_t z_bytes, bool write);
void add_sup_page (struct sup_page *page);
struct sup_page* get_sup_page (void *addr);
void delete_sup_page (struct sup_page *page);
void destroy_sup_page_table (void);

#endif /* vm/page.h */
