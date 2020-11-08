#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_
#include "common.h"
// kernel print use UART
int printk(const char *fmt, ...);

// user print use screen buffer
int printf(const char *fmt, ...);
int kprintk(const char *fmt, ...);
int kprintf(const char *fmt, ...);
#endif
