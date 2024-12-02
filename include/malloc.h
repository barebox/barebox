/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MALLOC_H
#define __MALLOC_H

#include <linux/compiler.h>
#include <types.h>

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

static inline bool want_init_on_alloc(void)
{
	return IS_ENABLED(CONFIG_INIT_ON_ALLOC_DEFAULT_ON);
}

static inline bool want_init_on_free(void)
{
	return IS_ENABLED(CONFIG_INIT_ON_FREE_DEFAULT_ON);
}

#endif /* __MALLOC_H */
