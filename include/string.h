/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STRING_H
#define __STRING_H

#include <linux/string.h>
#include <linux/minmax.h>

void *mempcpy(void *dest, const void *src, size_t count);
int strtobool(const char *str, int *val);
char *strsep_unescaped(char **, const char *);
char *stpcpy(char *dest, const char *src);
bool strends(const char *str, const char *postfix);

void *__default_memset(void *, int, __kernel_size_t);
void *__nokasan_default_memset(void *, int, __kernel_size_t);

void *__default_memcpy(void * dest,const void *src,size_t count);
void *__nokasan_default_memcpy(void * dest,const void *src,size_t count);

void *__default_memmove(void * dest,const void *src,size_t count);

char *parse_assignment(char *str);

int strverscmp(const char *a, const char *b);

char *strjoin(const char *separator, char **array, size_t len);

static inline int strcmp_ptr(const char *a, const char *b)
{
	return a && b ? strcmp(a, b) : compare3(a, b);
}

static inline bool streq_ptr(const char *a, const char *b)
{
	return strcmp_ptr(a, b) == 0;
}

static inline bool isempty(const char *s)
{
	return !s || s[0] == '\0';
}

static inline const char *nonempty(const char *s)
{
	return isempty(s) ? NULL : s;
}

#endif /* __STRING_H */
