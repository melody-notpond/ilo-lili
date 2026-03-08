#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// initialises the page allocator.
void init_page_alloc(void *start, size_t size);

// allocates a single page. currently, if no free pages are available, the
// allocator will panic. pages will have garbage data unless the clear flag is
// set.
void *alloc_page(bool clear);

// frees a previously allocated page.
void free_page(void *page);
