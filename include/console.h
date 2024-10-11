#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

void kputc(char c);

void kputsl(char *s, size_t len);

void kputs(char *s);

__attribute__ ((format(printf, 1, 2)))
int kprintf(const char *fmt, ...);

#endif /* CONSOLE_H */
