#ifndef __MALLOC_H
#define __MALLOC_H

#include <types.h>

/* Public routines */

void* malloc(size_t);
void free(void*);
void* realloc(void*, size_t);
void* memalign(size_t, size_t);
void* vallocc(size_t);
void* pvalloc(size_t);
void* calloc(size_t, size_t);
void cfree(void*);
int malloc_trim(size_t);
size_t malloc_usable_size(void*);
void malloc_stats(void);
int mallopt(int, int);
struct mallinfo mallinfo(void);
void *sbrk(ptrdiff_t increment);

#endif

