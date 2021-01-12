#ifndef INCLUDE_BITMAP_H_
#define INCLUDE_BITMAP_H_

/**
 * Initialize a bitmap (with all bits 0)
 * @param bitmap the address of bitmap
 * @param size the number of bits in the bitmap
 * @result the size of the memory of the bitmap
 */
int bitmap_init(void * bitmap, int size);

/**
 * Initialize a bitmap (with all bits 0)
 * @param bitmap the address of bitmap
 * @param size_byte the number of bits in the bitmap
 * @result the size of the memory of the bitmap
 */
int bitmap_init_byte(void * bitmap, int size_byte);

/**
 * Check whether a bit is 1
 * @param bitmap the address of bitmap
 * @param id the id of the bit
 * @return result
 */
int bitmap_check(void * bitmap, int id);

/**
 * Set a bit to 1
 * @param bitmap the address of bitmap
 * @param id the id of the bit
 */
void bitmap_set(void * bitmap, int id);

/**
 * clear a bit to 0
 * @param bitmap the address of bitmap
 * @param id the id of the bit
 */
void bitmap_clear(void * bitmap, int id);

/**
 * reverse a bit from 1 to 0 or from 0 to 1
 * @param bitmap the address of bitmap
 * @param id the id of the bit
 */
void bitmap_reverse(void * bitmap, int id);

/**
 * assign a bit
 * @param bitmap the address of bitmap
 * @param id the id of the bit
 * @param value the value of the bit
 */
void bitmap_assign(void * bitmap, int id, int value);

#endif
