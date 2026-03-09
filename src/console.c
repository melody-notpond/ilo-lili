#include <stdarg.h>

#include "console.h"
#include "drivers/opensbi.h"
#include "string.h"

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS   0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS       0
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS       0
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS       0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS      0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS   0

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

// prints a character.
void kputc(char c) {
  struct sbiret ret = sbi_debug_console_write_byte(c);

  // primitive error handling
  // we can't really print an error if printing just doesnt work...
  while (ret.error != SBI_SUCCESS);
}

// prints a string of the specified length
void knputs(char *s, size_t len) {
  struct sbiret ret = sbi_debug_console_write(len, ((intptr_t) s) & 0xffffffff,
    ((intptr_t) s) >> 32);

  while (ret.error != SBI_SUCCESS);
}

// prints a string
void kputs(char *s) {
  knputs(s, strlen(s));
}

// wrapper for kputc for usage with nanoprintf
void kputc_npf(int c, void *ctx) {
  (void) ctx;
  kputc(c);
}

// internal panic function.
__attribute__((noreturn))
__attribute__ ((format(printf, 4, 5)))
void _panic(const char *file, int line, const char *func, const char *fmt, ...)
{
  kprintf("%s:%i(%s) kernel panic! ", file, line, func);
  va_list va;
  va_start(va, fmt);
  npf_vpprintf(kputc_npf, NULL, fmt, va);
  va_end(va);
  kputc('\n');
  while(1);
}

// internal log function.
__attribute__ ((format(printf, 4, 5)))
void _log(const char *file, int line, const char *func, const char *fmt, ...) {
  kprintf("%s:%i(%s) ", file, line, func);
  va_list va;
  va_start(va, fmt);
  npf_vpprintf(kputc_npf, NULL, fmt, va);
  va_end(va);
  kputc('\n');
}

// printf lmao
__attribute__ ((format(printf, 1, 2)))
int kprintf(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  npf_vpprintf(kputc_npf, NULL, fmt, va);
  va_end(va);
  return 0;
}

// calculates the base 16 log of a number.
static int _log16(size_t n) {
  int i = 0;
  while (n) {
    n >>= 4;
    i++;
  }
  return i;
}

// print a string, replacing nonprintable characters with a period.
void kputx(void *data, size_t size) {
  char *s = (char *) data;
  for (size_t i = 0; i < size; i++) {
    if (32 <= s[i] && s[i] <= 127)
      kputc(s[i]);
    else kputc('.');
  }
}

// prints a hexdump of the provided data.
void khexdump(void* data, size_t size) {
  unsigned int num_zeros = _log16(size);
  unsigned char* data_char = (unsigned char*) data;

  for (unsigned long long i = 0; i < (size + 15) / 16; i++) {
    // print out buffer zeroes
    unsigned int num_zeros_two = num_zeros - _log16(i) - 1;
    for (unsigned int j = 0; j < num_zeros_two; j++) {
      kprintf("%x", 0);
    }

    kprintf("%llx    ", i * 16);

    for (int j = 0; j < 16; j++) {
      unsigned long long index = i * 16 + j;

      if (index >= size)
        kputs("   ");
      else {
        // print out the value
        if (data_char[index] < 16)
            kprintf("%x", 0);
        kprintf("%x ", data_char[index]);
      }
    }

    kputs("    |");
    for (int j = 0; j < 16; j++) {
      unsigned long long index = i * 16 + j;
      if (index >= size)
        kputc('.');
      else if (32 <= data_char[index] && data_char[index] < 127)
        kputc(data_char[index]);
      else
        kputc('.');
    }

    kputs("|\n");
  }
}
