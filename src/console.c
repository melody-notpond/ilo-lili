#include <stdarg.h>

#include "console.h"
#include "drivers/opensbi.h"
#include "string.h"

void kputc(char c) {
    struct sbiret ret = sbi_debug_console_write_byte(c);

    // primitive error handling
    // we can't really print an error if printing just doesnt work...
    while (ret.error != SBI_SUCCESS);
}

void kputsl(char *s, size_t len) {
    struct sbiret ret = sbi_debug_console_write(
        len,
        ((intptr_t) s) & 0xffffffff,
        ((intptr_t) s) >> 32);

    while (ret.error != SBI_SUCCESS);
}

void kputs(char *s) {
    kputsl(s, strlen(s));
}


int kvprintf(const char* format, va_list va) {
    for (; *format; format++) {
        if (*format == '%') {
            format++;

            switch (*format) {
                case 'c':
                    kputc(va_arg(va, int));
                    break;

                case 'p': {
                    intptr_t p = (intptr_t) va_arg(va, void*);
                    if (p == 0) {
                        kputs("(null)");
                        break;
                    }

                    kputs("0x");
                    static const size_t buffer_size = 16;
                    char buffer[buffer_size];
                    size_t len = 0;
                    while (p != 0) {
                        len++;
                        buffer[buffer_size - len] = "0123456789abcdef"[p % 16];
                        p /= 16;
                    }

                    for (size_t i = buffer_size - len; i < buffer_size; i++) {
                        kputc(buffer[i]);
                    }
                    break;
                }

                case 's': {
                    char* s = va_arg(va, char*);
                    kputs(s);
                    break;
                }

                case 'l':
                    format++;
                    if (*format == 'l'){
                        format++;
                    }

                    if (*format != 'x') {
                        va_arg(va, int64_t);
                        format--;
                        break;
                    }

                    goto _kvprintf_fallthrough;

                case 'x':
_kvprintf_fallthrough: {
                    int64_t x;
                    x = va_arg(va, int64_t);

                    if (x == 0) {
                        kputc('0');
                        break;
                    }

                    static const size_t buffer_size = 16;
                    char buffer[buffer_size];
                    size_t len = 0;
                    while (x != 0) {
                        len++;
                        buffer[buffer_size - len] = "0123456789abcdef"[x % 16];
                        x /= 16;
                    }

                    for (size_t i = buffer_size - len; i < buffer_size; i++) {
                        kputc(buffer[i]);
                    }

                    break;
                }

                case '%':
                    kputc('%');
                    break;

                default:
                    break;
            }

        // Regular characters
        } else {
            kputc(*format);
        }
    }

    return 0;
}

__attribute__ ((format(printf, 1, 2)))
int kprintf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    kvprintf(fmt, va);
    va_end(va);
    return 0;
}

int _log16(size_t n) {
    int i = 0;
    while (n) {
        n >>= 4;
        i++;
    }
    return i;
}

void khexdump(void* data, size_t size) {
    unsigned int num_zeros = _log16(size);
    unsigned char* data_char = (unsigned char*) data;

    for (unsigned long long i = 0; i < (size + 15) / 16; i++) {
        // Print out buffer zeroes
        unsigned int num_zeros_two = num_zeros - _log16(i) - 1;
        for (unsigned int j = 0; j < num_zeros_two; j++) {
            kprintf("%x", 0);
        }

        // Print out label
        kprintf("%llx    ", i * 16);

        // Print out values
        for (int j = 0; j < 16; j++) {
            unsigned long long index = i * 16 + j;

            if (index >= size)
                kputs("   ");
            else {
                // Print out the value
                if (data_char[index] < 16)
                    kprintf("%x", 0);
                kprintf("%x ", data_char[index]);
            }
        }

        kputs("    |");

        // Print out characters
        for (int j = 0; j < 16; j++) {
            unsigned long long index = i * 16 + j;

            // Skip characters if the index is greater than the number of characters to dump
            if (index >= size)
                kputc('.');

            // Print out printable characters
            else if (32 <= data_char[index] && data_char[index] < 127)
                kputc(data_char[index]);

            // Nonprintable characters are represented by a period (.)
            else
                kputc('.');
        }

        kputs("|\n");
    }
}
