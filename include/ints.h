#ifndef INTS_H
#define INTS_H

// converts little endian to big endian
#define le2be32(n) \
    (((n) & 0xff) << 24 | ((n) & 0xff00) << 8 | \
    ((n) & 0xff0000) >> 8 | ((n) & 0xff000000) >> 24)

// converts big endian to little endian
#define be2le32(n) \
    (((n) & 0xff) << 24 | ((n) & 0xff00) << 8 | \
    ((n) & 0xff0000) >> 8 | ((n) & 0xff000000) >> 24)

// TODO: endianness check

// converts native endian to big endian
#define nv2be32(n) \
    (((n) & 0xff) << 24 | ((n) & 0xff00) << 8 | \
    ((n) & 0xff0000) >> 8 | ((n) & 0xff000000) >> 24)

// converts big endian to native endian
#define be2nv32(n) \
    (((n) & 0xff) << 24 | ((n) & 0xff00) << 8 | \
    ((n) & 0xff0000) >> 8 | ((n) & 0xff000000) >> 24)

#define align2(t, p) \
    ((t *) (((intptr_t) (p) + 1) & ~1))

#define align4(t, p) \
    ((t *) (((intptr_t) (p) + 3) & ~3))

#define align8(t, p) \
    ((t *) (((intptr_t) (p) + 7) & ~7))

#define align16(t, p) \
    ((t *) (((intptr_t) (p) + 15) & ~15))

#endif /* INTS_H */
