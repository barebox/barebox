#ifndef __XFUNCS_H
#define __XFUNCS_H

#include <linux/types.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xzalloc(size_t size);
char *xstrdup(const char *s);
void* xmemalign(size_t alignment, size_t bytes);

#endif /* __XFUNCS_H */
