#include "string.h"

unsigned long strlen(const char *str) {
  unsigned long len = 0;
  while (str[len])
      len++;
  return len;
}

bool streq(const char *a, const char *b) {
  while (*a && *b) {
    if (*a++ != *b++)
      return false;
  }

  return *a == *b;
}
