#include "console.h"
#include "drivers/devicetree.h"
#include "interrupts.h"

extern int stack_top;

trap_t boot_trap = {
  .interrupt_stack = (int64_t) &stack_top
};

void kinit(unsigned long long hartid, void *fdt) {
  (void) hartid;

  devicetree tree = fdt_validate(fdt);
  if (!tree) {
    kprintf("invalid device tree!\n");
    while(1);
  }

  kprintf("%p is a valid device tree\n", tree);
  kprintf("%p has %x entries in the memory reservation block\n", tree, fdt_count_mem_reserve_entries(tree));
  khexdump(fdt, 32);

  // test node iteration
  for ( fdt_node node = fdt_root_node(tree)
      ; fdt_node_valid(node)
      ; node = fdt_node_iter(node) ) {
    kprintf("theres a device tree node called '%s'\n", fdt_node_name(node));
  }

  kprintf("done iterating over device tree nodes");

  while(1);
}
