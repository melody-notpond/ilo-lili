#include "string.h"

unsigned long strlen(const char *str) {
    unsigned long len = 0;
    while (str[len])
        len++;
    return len;
}
