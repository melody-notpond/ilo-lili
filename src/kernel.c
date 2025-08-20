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

  fdt_dump(tree);

  char *path = "/chosen";
  char *prop_name = "stdout-path";
  fdt_node node = fdt_node_path(tree, path);
  if (fdt_node_valid(node)) {
    kprintf("%s exists and is called %s!\n", path, fdt_node_name(node));
    fdt_prop prop = fdt_node_prop(tree, node, prop_name);

    if (fdt_prop_valid(prop)) {
      kprintf ("%s exists in %s!\n", prop_name, path);
    } else {
      kprintf ("%s does not exist in %s\n", prop_name, path);
    }
  } else {
    kprintf("%s does not exist\n", path);
  }

  while(1);
}
