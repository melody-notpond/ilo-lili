.section .text
.global _start

# a0 - current hart id
# a1 - pointer to flattened device tree

_start:
  # init stack pointer
  la sp, stack_top
  mv fp, sp

  # init gp
  .option push
  .option norelax
  lla gp, __global_pointer$
  .option pop

  # init interrupt vector
  la t0, handle_trap
  csrw stvec, t0

  # sscratch is set to the trap_t structure
  la t0, boot_trap
  csrw sscratch, t0

  # init csrs to enable interrupts in supervisor mode
  li t0, 0x022        # ssie (software interrupt enable) = 1, nothing else
  csrw sie, t0
  li t0, 0x22         # sie (supervisor interrupt enable) = 1
  csrs sstatus, t0

  j kinit

_loop:
  j _loop
