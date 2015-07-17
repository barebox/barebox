#ifndef __XFUNCS_H
#define __XFUNCS_H

#include <linux/types.h>
#include <stdarg.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xzalloc(size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t size);
void* xmemalign(size_t alignment, size_t bytes);
void* xmemdup(const void *orig, size_t size);
char *xasprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
char *xvasprintf(const char *fmt, va_list ap);

#endif /* __XFUNCS_H */
