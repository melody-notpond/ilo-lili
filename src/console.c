#include <stdarg.h>

#include "console.h"
#include "opensbi.h"
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
