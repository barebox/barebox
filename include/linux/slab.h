/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <dma.h>
#include <linux/overflow.h>
#include <linux/string.h>

#define SLAB_CONSISTENCY_CHECKS	0
#define SLAB_RED_ZONE		0
#define SLAB_POISON		0
#define SLAB_HWCACHE_ALIGN	0
#define SLAB_CACHE_DMA		0
#define SLAB_STORE_USER		0
#define SLAB_PANIC		0
#define SLAB_TYPESAFE_BY_RCU	0
#define SLAB_MEM_SPREAD		0
#define SLAB_TRACE		0
#define SLAB_DEBUG_OBJECTS	0
#define SLAB_NOLEAKTRACE	0
#define SLAB_FAILSLAB		0
#define SLAB_ACCOUNT		0
#define SLAB_KASAN		0
#define SLAB_RECLAIM_ACCOUNT	0
#define SLAB_TEMPORARY		0

/* unused in barebox, just bogus values */
#define GFP_KERNEL	0
#define GFP_NOFS	0
#define GFP_USER	0
#define __GFP_NOWARN	0

static inline void *kmalloc(size_t size, gfp_t flags)
{
	return dma_alloc(size);
}

struct kmem_cache {
	unsigned int size;
	void (*ctor)(void *);
};

static inline
struct kmem_cache *kmem_cache_create(const char *name, unsigned int size,
                        unsigned int align, slab_flags_t flags,
                        void (*ctor)(void *))
{
	struct kmem_cache *cache = kmalloc(sizeof(*cache), GFP_KERNEL);

	if (!cache)
		return NULL;

	cache->size = size;
	cache->ctor = ctor;

	return cache;
}

static inline void kmem_cache_destroy(struct kmem_cache *cache)
{
	dma_free(cache);
}

static inline void kfree(const void *mem)
{
	dma_free((void *)mem);
}

static inline void *kmem_cache_alloc(struct kmem_cache *cache, gfp_t flags)
{
	void *mem = kmalloc(cache->size, flags);

	if (!mem)
		return NULL;

	if (cache->ctor)
		cache->ctor(mem);

	return mem;
}


static inline void kmem_cache_free(struct kmem_cache *cache, void *mem)
{
	kfree(mem);
}

static inline void *kzalloc(size_t size, gfp_t flags)
{
	return dma_zalloc(size);
}

/**
 * kmalloc_array - allocate memory for an array.
 * @n: number of elements.
 * @size: element size.
 * @flags: the type of memory to allocate (see kmalloc).
 */
static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	return kmalloc(size_mul(n, size), flags);
}

static inline void *kcalloc(size_t n, size_t size, gfp_t flags)
{
	return dma_zalloc(size_mul(n, size));
}

static inline void *krealloc(void *ptr, size_t size, gfp_t flags)
{
	return realloc(ptr, size);
}

static inline char *kstrdup(const char *str, gfp_t flags)
{
	return strdup(str);
}

#define kstrdup_const(str, flags) strdup(str)
#define kfree_const(ptr) kfree((void *)ptr)

#endif /* _LINUX_SLAB_H */
