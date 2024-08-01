/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __XFUNCS_H
#define __XFUNCS_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <stdarg.h>
#include <wchar.h>

void *xmalloc(size_t size) __xalloc_size(1);
void *xrealloc(void *ptr, size_t size) __xrealloc_size(2);
void *xzalloc(size_t size) __xalloc_size(1);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t size) __returns_nonnull;
void* xmemalign(size_t alignment, size_t bytes) __xalloc_size(2);
void* xmemdup(const void *orig, size_t size) __returns_nonnull;
char *xasprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2))) __returns_nonnull;
char *xvasprintf(const char *fmt, va_list ap) __returns_nonnull;

wchar_t *xstrdup_wchar(const wchar_t *src);
wchar_t *xstrdup_char_to_wchar(const char *src);
char *xstrdup_wchar_to_char(const wchar_t *src);

#endif /* __XFUNCS_H */
