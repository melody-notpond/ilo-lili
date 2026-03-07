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

  kprintf("%p is a valid device tree\n", (void *) tree);

  fdt_dump(tree);

  char *name = "virtio_mmio";
  for ( fdt_node node = fdt_node_search_iter(tree, NULL, name)
      ; fdt_node_valid(node)
      ; node = fdt_node_search_iter(tree, node, name) ) {
    kprintf("we have a %s node called %s\n", name, fdt_node_name(node));
  }

  kprintf("done\n");

  while(1);
}
