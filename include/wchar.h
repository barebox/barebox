/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __WCHAR_H
#define __WCHAR_H

#include <linux/types.h>
#include <linux/stddef.h>

wchar_t *strdup_wchar(const wchar_t *src);

char *strcpy_wchar_to_char(char *dst, const wchar_t *src);

wchar_t *strcpy_char_to_wchar(wchar_t *dst, const char *src);

wchar_t *strdup_char_to_wchar(const char *src);

char *strdup_wchar_to_char(const wchar_t *src);

size_t wcslen(const wchar_t *s);

size_t wcsnlen(const wchar_t *s, size_t maxlen);

#define MB_CUR_MAX 4

int mbtowc(wchar_t *pwc, const char *s, size_t n);
int wctomb(char *s, wchar_t wc);

int wcscmp (const wchar_t *s1, const wchar_t *s2);
int wcsncmp (const wchar_t *s1, const wchar_t *s2, size_t n);

#endif /* __WCHAR_H */
