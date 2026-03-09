#include "pages.h"
#include "console.h"
#include "mmu.h"

// TODO: SYNCHRONISATION

// braindead simple way of allocating pages a la xv6
// TODO: switch to buddy allocator
// https://www.kernel.org/doc/gorman/html/understand/understand009.html

static struct next_block {
  struct next_block *next;
} *free_list;

// initialises the page allocator.
void init_page_alloc(void *start, size_t size) {
  free_list = (struct next_block *) PAGE_ALIGN(start);
  size_t count = (size + (size_t) start - (size_t) free_list) >> PAGE_BITS;

  struct next_block *block = free_list;
  for (size_t i = 0; i < count - 1; i++) {
    block->next = (struct next_block *) ((uintptr_t) block + PAGE_SIZE);
    block = block->next;
  }

  block->next = NULL;
}

// allocates a single physical page. currently, if no free pages are available,
// the allocator will panic. pages will have garbage data unless the clear flag
// is set.
void *alloc_page(bool clear) {
  // TODO: free least recently used pages
  if (!free_list)
    panic("ran out of free pages");

  struct next_block *block = make_accessible(free_list);
  free_list = block->next;

  block->next = NULL;
  if (clear) {
    uint64_t *p = (uint64_t *) block;
    for (int i = 0; i < PAGE_SIZE / (int) sizeof(uint64_t); i++) {
      *p = 0;
      p++;
    }
  }

  return virt2phys(block);
}

// frees a previously allocated page.
void free_page(void *page) {
  struct next_block *block = (struct next_block *) page;
  ((struct next_block *) make_accessible(block))->next = free_list;
  free_list = block;
}
