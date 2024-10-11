.section .text
.global _start

# a0 - current hart id
# a1 - pointer to flattened device tree

_start:
    # initialise stack pointer
    la sp, stack_top
    mv fp, sp

    # init gp
    .option push
    .option norelax
    lla gp, __global_pointer$
    .option pop

    j kinit

_loop:
    j _loop
