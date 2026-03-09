#include <stdbool.h>

#include "console.h"
#include "interrupts.h"

trap_t *interrupt_handler(uint64_t cause, trap_t *trap) {
  if (cause & 0x8000000000000000) {
    cause &= 0x7fffffffffffffff;
    panic("async interrupt (%li) at %lx", cause, trap->pc);
  } else {
    panic("sync interrupt (%li) at %lx", cause, trap->pc);
  }

  return trap;
}
