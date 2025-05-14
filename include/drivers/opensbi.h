#ifndef OPENSBI_H
#define OPENSBI_H

#include <stdint.h>

// see the below link for documentation:
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/riscv-sbi.adoc

typedef enum {
    SBI_SUCCESS                =   0,
    SBI_ERR_FAILED             = - 1,
    SBI_ERR_NOT_SUPPORTED      = - 2,
    SBI_ERR_INVALID_PARAM      = - 3,
    SBI_ERR_DENIED             = - 4,
    SBI_ERR_INVALID_ADDRESS    = - 5,
    SBI_ERR_ALREADY_AVAILABLE  = - 6,
    SBI_ERR_ALREADY_STARTED    = - 7,
    SBI_ERR_ALREADY_STOPPED    = - 8,
    SBI_ERR_NO_SHMEM           = - 9,
    SBI_ERR_INVALID_STATE      = -10,
    SBI_ERR_BAD_RANGE          = -11,
    SBI_ERR_TIMEOUT            = -12,
    SBI_ERR_IO                 = -13,
} sbi_error;

struct sbiret {
    sbi_error error;
    long value;
};

struct sbiret sbi_debug_console_write(
    unsigned long num_bytes,
    unsigned long base_addr_lo,
    unsigned long base_addr_hi
);

struct sbiret sbi_debug_console_read(
    unsigned long num_bytes,
    unsigned long base_addr_lo,
    unsigned long base_addr_hi
);

struct sbiret sbi_debug_console_write_byte(uint8_t byte);

#endif /* OPENSBI_H */
