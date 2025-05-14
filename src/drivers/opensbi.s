.section .text

.global sbi_debug_console_write
.global sbi_debug_console_read
.global sbi_debug_console_write_byte

# eids are stored in a7
# fids are stored in a6

sbi_debug_console_write:
    li a6, 0
    li a7, 0x4442434E
    ecall
    ret

sbi_debug_console_read:
    li a6, 1
    li a7, 0x4442434E
    ecall
    ret

sbi_debug_console_write_byte:
    li a6, 2
    li a7, 0x4442434E
    ecall
    ret

