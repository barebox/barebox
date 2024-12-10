/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MALLOC_H
#define __MALLOC_H

#include <linux/compiler.h>
#include <types.h>

#if IN_PROPER
void *malloc(size_t) __alloc_size(1);
size_t malloc_usable_size(void *);
void free(void *);
void free_sensitive(void *);
void *realloc(void *, size_t) __realloc_size(2);
void *memalign(size_t, size_t) __alloc_size(2);
void *calloc(size_t, size_t) __alloc_size(1, 2);
void malloc_stats(void);
void *sbrk(ptrdiff_t increment);

int mem_malloc_is_initialized(void);
#else
static inline void *malloc(size_t nbytes)
{
	return NULL;
}
static inline void free(void *buf)
{
}
static inline void free_sensitive(void *buf)
{
}
static inline void *realloc(void *orig, size_t nbytes)
{
	return NULL;
}
static inline void *memalign(size_t align, size_t size)
{
	return NULL;
}
static inline void *calloc(size_t num, size_t size)
{
	return NULL;
}
static inline void malloc_stats(void)
{
}
static inline void *sbrk(ptrdiff_t increment)
{
	return NULL;
}

static inline int mem_malloc_is_initialized(void)
{
	return 0;
}
#endif

#endif /* __MALLOC_H */
