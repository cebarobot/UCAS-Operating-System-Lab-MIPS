#ifndef INCLUDE_BITMAP_
#define INCLUDE_BITMAP_

int bitmap_init(void * bitmap, int size);

int bitmap_init_byte(void * bitmap, int size_byte);

int bitmap_check(void * bitmap, int id);

void bitmap_set(void * bitmap, int id);

void bitmap_clear(void * bitmap, int id);

void bitmap_reverse(void * bitmap, int id);

void bitmap_assign(void * bitmap, int id, int value);

#endif