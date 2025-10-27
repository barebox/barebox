/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MALLOC_H
#define __MALLOC_H

#include <linux/compiler.h>
#include <types.h>

#define MALLOC_SHIFT_MAX	30
#define MALLOC_MAX_SIZE		(1UL << MALLOC_SHIFT_MAX)

/*
 * ZERO_SIZE_PTR will be returned for zero sized kmalloc requests.
 *
 * Dereferencing ZERO_SIZE_PTR will lead to a distinct access fault.
 *
 * ZERO_SIZE_PTR can be passed to free though in the same way that NULL can.
 * Both make free a no-op.
 */
#define ZERO_SIZE_PTR ((void *)16)

#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= \
				(unsigned long)ZERO_SIZE_PTR)

#ifdef CONFIG_MALLOC_TLSF
void *malloc_add_pool(void *mem, size_t bytes);
void malloc_register_store(void (*cb)(size_t bytes));
bool malloc_store_is_registered(void);
#else
static inline bool malloc_store_is_registered(void) { return false; }
#endif

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

static inline bool want_init_on_alloc(void)
{
	return IS_ENABLED(CONFIG_INIT_ON_ALLOC_DEFAULT_ON);
}

static inline bool want_init_on_free(void)
{
	return IS_ENABLED(CONFIG_INIT_ON_FREE_DEFAULT_ON);
}

#endif /* __MALLOC_H */
