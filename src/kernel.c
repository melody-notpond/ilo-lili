#include "console.h"

void kinit(unsigned long long hartid, void *fdt) {
    (void) hartid;
    (void) fdt;

    kprintf("haiiii :3\n%s %x", "amogus", 0x1234abcd);
    while(1);
}
