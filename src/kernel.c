#include "console.h"
#include "drivers/devicetree.h"
#include "interrupts.h"
#include "pages.h"

extern int stack_top;
extern int pages_bottom;

trap_t boot_trap = {
  .interrupt_stack = (int64_t) &stack_top
};

void kinit(unsigned long long hartid, void *fdt) {
  (void) hartid;

  devicetree tree = fdt_validate(fdt);
  if (!tree) {
    panic("kinit", "invalid device tree!");
  }

  kprintf("%p is a valid device tree and has %x memory reservations\n",
    (void *) tree, fdt_count_mem_reserve_entries(tree));

  fdt_dump(tree);

  // TODO: this stuff is hardcoded because im lazy but we should be reading
  // this from the device tree instead
  const size_t raw_mem_start = 0x80000000;
  void *mem_start = &pages_bottom;
  // 256mb
  const size_t mem_size = 0x10000000 - ((size_t) mem_start - raw_mem_start);

  init_page_alloc(mem_start, mem_size);

  while(1);
}
