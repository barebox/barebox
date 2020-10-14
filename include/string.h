/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STRING_H
#define __STRING_H

#include <linux/string.h>

int strtobool(const char *str, int *val);
char *strsep_unescaped(char **, const char *);
char *stpcpy(char *dest, const char *src);

void *__default_memset(void *, int, __kernel_size_t);
void *__nokasan_default_memset(void *, int, __kernel_size_t);

void *__default_memcpy(void * dest,const void *src,size_t count);
void *__nokasan_default_memcpy(void * dest,const void *src,size_t count);


#endif /* __STRING_H */
