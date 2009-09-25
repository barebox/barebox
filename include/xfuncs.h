#ifndef __XFUNCS_H
#define __XFUNCS_H

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xzalloc(size_t size);
char *xstrdup(const char *s);

#endif /* __XFUNCS_H */
