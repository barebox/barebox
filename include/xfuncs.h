/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __XFUNCS_H
#define __XFUNCS_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bug.h>
#include <stdarg.h>
#include <wchar.h>
#include <malloc.h>

#if IN_PROPER || !defined(__BAREBOX__)
void *xmalloc(size_t size) __xalloc_size(1);
void *xrealloc(void *ptr, size_t size) __xrealloc_size(2);
void *xzalloc(size_t size) __xalloc_size(1);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t size) __returns_nonnull;
void* xmemalign(size_t alignment, size_t bytes) __xalloc_size(2);
void* xmemdup(const void *orig, size_t size) __returns_nonnull;
char *xasprintf(const char *fmt, ...) __printf(1, 2) __returns_nonnull;
char *xvasprintf(const char *fmt, va_list ap) __returns_nonnull;

wchar_t *xstrdup_wchar(const wchar_t *src);
wchar_t *xstrdup_char_to_wchar(const char *src);
char *xstrdup_wchar_to_char(const wchar_t *src);

#else

static inline void *xmalloc(size_t size) { BUG(); }
static inline void *xrealloc(void *ptr, size_t size) { BUG(); }
static inline void *xzalloc(size_t size) { BUG(); }
static inline char *xstrdup(const char *s) { BUG(); }
static inline char *xstrndup(const char *s, size_t size) { BUG(); }
static inline void* xmemalign(size_t alignment, size_t bytes) { BUG(); }
static inline void* xmemdup(const void *orig, size_t size) { BUG(); }
static inline __printf(1, 2) char *xasprintf(const char *fmt, ...) { BUG(); }
static inline char *xvasprintf(const char *fmt, va_list ap) { BUG(); }

static inline wchar_t *xstrdup_wchar(const wchar_t *src) { BUG(); }
static inline wchar_t *xstrdup_char_to_wchar(const char *src) { BUG(); }
static inline char *xstrdup_wchar_to_char(const wchar_t *src) { BUG(); }
#endif

#endif /* __XFUNCS_H */
