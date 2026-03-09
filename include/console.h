#pragma once

#include <stddef.h>

// prints a character.
void kputc(char c);

// prints a string of the specified length
void knputs(char *s, size_t len);

// prints a string
void kputs(char *s);

// kernel panic.
#define panic(fmt, ...) _panic(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// kernel log.
#define log(fmt, ...) _log(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// internal panic function.
__attribute__((noreturn))
__attribute__ ((format(printf, 4, 5)))
void _panic(const char *file, int line, const char *func, const char *fmt, ...);

// internal log function.
__attribute__ ((format(printf, 4, 5)))
void _log(const char *file, int line, const char *func, const char *fmt, ...);

// printf lmao
__attribute__ ((format(printf, 1, 2)))
int kprintf(const char *fmt, ...);

// print a string, replacing nonprintable characters with a period.
void kputx(void *data, size_t size);

// prints a hexdump of the provided data.
void khexdump(void* data, size_t size);
