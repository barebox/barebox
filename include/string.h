#ifndef __STRING_H
#define __STRING_H

#include <linux/string.h>

void *memdup(const void *, size_t);
int strtobool(const char *str, int *val);

void *__default_memset(void *, int, __kernel_size_t);
void *__default_memcpy(void * dest,const void *src,size_t count);

#endif /* __STRING_H */
