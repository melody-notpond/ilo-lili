#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// number of byts in a page
#define PAGE_SIZE 4096

// bit mask for index within a page in an address
#define PAGE_MASK 0xfff

// length of page bitmask
#define PAGE_BITS 12

// aligns an address to the last page
#define PAGE_ALIGN_PREV(addr) ((void *) ((uintptr_t) addr & ~PAGE_MASK))

// aligns an address to the next page
#define PAGE_ALIGN(addr) \
  ((void *) ((((uintptr_t) addr) + PAGE_MASK) & ~PAGE_MASK))

// initialises the page allocator.
void init_page_alloc(void *start, size_t size);

// allocates a single page. currently, if no free pages are available, the
// allocator will panic. pages will have garbage data unless the clear flag is
// set.
void *alloc_page(bool clear);

// frees a previously allocated page.
void free_page(void *page);
