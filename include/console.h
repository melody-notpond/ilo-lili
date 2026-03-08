#pragma once

#include <stddef.h>

void kputc(char c);

void kputsl(char *s, size_t len);

void kputs(char *s);

__attribute__((noreturn))
void panic(char *func_name, char *msg);

__attribute__ ((format(printf, 1, 2)))
int kprintf(const char *fmt, ...);

// print a string, replacing nonprintable characters with a period.
void kputx(void *data, size_t size);

void khexdump(void* data, size_t size);
