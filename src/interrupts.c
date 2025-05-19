#include <stdbool.h>

#include "console.h"
#include "interrupts.h"

trap_t *interrupt_handler(uint64_t cause, trap_t *trap) {
  if (cause & 0x8000000000000000) {
    cause &= 0x7fffffffffffffff;
    kprintf("async cause: %lx\ntrap location: %lx\ntrap caller: %lx\n",
      cause, trap->pc, trap->xs[REGISTER_RA]);
  } else {
    kprintf("sync cause: %lx\ntrap location: %lx\ntrap caller: %lx\n",
      cause, trap->pc, trap->xs[REGISTER_RA]);
  }

  while (true);

  return trap;
}
