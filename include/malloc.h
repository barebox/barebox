#ifndef __MALLOC_H
#define __MALLOC_H

#include <types.h>

void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void *memalign(size_t, size_t);
void *calloc(size_t, size_t);
void malloc_stats(void);
void *sbrk(ptrdiff_t increment);

int mem_malloc_is_initialized(void);

#endif /* __MALLOC_H */
