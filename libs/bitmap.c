#include "bitmap.h"
#include "string.h"

int bitmap_init(void * bitmap, int size) {
    int num_byte = size / 8;
    memset(bitmap, 0, num_byte);
    return num_byte;
}

int bitmap_init_byte(void * bitmap, int size_byte) {
    memset(bitmap, 0, size_byte);
    return size_byte;
}

int bitmap_check(void * bitmap, int id) {
    uint32_t * array_32 = bitmap;
    int group = id / 32;
    int offset = id % 32;
    return ( array_32[group] & ((uint32_t)1 << offset) ) != 0;
}

void bitmap_set(void * bitmap, int id) {
    uint32_t * array_32 = bitmap;
    int group = id / 32;
    int offset = id % 32;
    array_32[group] |= ((uint32_t)1 << offset);
}

void bitmap_clear(void * bitmap, int id) {
    uint32_t * array_32 = bitmap;
    int group = id / 32;
    int offset = id % 32;
    array_32[group] &= ~((uint32_t)1 << offset);
}

void bitmap_reverse(void * bitmap, int id) {
    uint32_t * array_32 = bitmap;
    int group = id / 32;
    int offset = id % 32;
    array_32[group] ^= ((uint32_t)1 << offset);
}

void bitmap_assign(void * bitmap, int id, int value) {
    uint32_t * array_32 = bitmap;
    int group = id / 32;
    int offset = id % 32;
    if (value) {
        array_32[group] |= ((uint32_t)1 << offset);
    } else {
        array_32[group] &= ~((uint32_t)1 << offset);
    }
}
