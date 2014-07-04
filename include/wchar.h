#ifndef __WCHAR_H
#define __WCHAR_H

#include <linux/types.h>

typedef u16 wchar_t;

char *strcpy_wchar_to_char(char *dst, const wchar_t *src);

wchar_t *strcpy_char_to_wchar(wchar_t *dst, const char *src);

wchar_t *strdup_char_to_wchar(const char *src);

char *strdup_wchar_to_char(const wchar_t *src);

size_t wcslen(const wchar_t *s);

#endif /* __WCHAR_H */
