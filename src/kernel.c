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

  // test node iteration
  for ( fdt_node node = fdt_root_node(tree)
      ; fdt_node_valid(node)
      ; node = fdt_node_iter(node) ) {
    kprintf("theres a device tree node called '%s'\n", fdt_node_name(node));

    for ( fdt_prop prop = fdt_prop_iter(node, NULL)
        ; fdt_prop_valid(prop)
        ; prop = fdt_prop_iter(node, prop)) {
      kprintf("  which has property called '%s'\n", fdt_prop_name(tree, prop));
    }
  }

  kprintf("done iterating over device tree nodes\n");

  while(1);
}
