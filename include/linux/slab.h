/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <dma.h>
#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/overflow.h>
#include <linux/string.h>
#include <linux/gfp.h>

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
	dma_free_const(mem);
}

#define kfree_const(ptr) dma_free_const(ptr)

static inline void kfree_sensitive(const void *objp)
{
	dma_free_sensitive((void *)objp);
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

static inline char *kstrdup(const char *str, gfp_t flags)
{
	return strdup(str);
}

#define kstrdup_const(str, flags) strdup_const(str)

DEFINE_FREE(kfree, void *, if (!IS_ERR_OR_NULL(_T)) kfree(_T))
DEFINE_FREE(kfree_sensitive, void *, if (!IS_ERR_OR_NULL(_T)) kfree_sensitive(_T))

static inline void *kvmalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags);
}

static inline void *kvzalloc(size_t size, gfp_t flags)
{
	return kzalloc(size, flags);
}

static inline void kvfree(const void *mem)
{
	kfree(mem);
}

/**
 * __alloc_objs - Allocate objects of a given type using
 * @KMALLOC: which size-based kmalloc wrapper to allocate with.
 * @GFP: GFP flags for the allocation.
 * @TYPE: type to allocate space for.
 * @COUNT: how many @TYPE objects to allocate.
 *
 * Returns: Newly allocated pointer to (first) @TYPE of @COUNT-many
 * allocated @TYPE objects, or NULL on failure.
 */
#define __alloc_objs(KMALLOC, GFP, TYPE, COUNT)				\
({									\
	const size_t __obj_size = size_mul(sizeof(TYPE), COUNT);	\
	(TYPE *)KMALLOC(__obj_size, GFP);				\
})

/**
 * kmalloc_obj - Allocate a single instance of the given type
 * @VAR_OR_TYPE: Variable or type to allocate.
 * @...: optional GFP flags for the allocation (GFP_KERNEL when not specified).
 *
 * Returns: newly allocated pointer to a @VAR_OR_TYPE on success, or NULL
 * on failure.
 */
#define kmalloc_obj(VAR_OR_TYPE, ...) \
	__alloc_objs(kmalloc, default_gfp(__VA_ARGS__), typeof(VAR_OR_TYPE), 1)

/**
 * kmalloc_objs - Allocate an array of the given type
 * @VAR_OR_TYPE: Variable or type to allocate an array of.
 * @COUNT: How many elements in the array.
 * @...: optional GFP flags for the allocation (GFP_KERNEL when not specified).
 *
 * Returns: newly allocated pointer to array of @VAR_OR_TYPE on success,
 * or NULL on failure.
 */
#define kmalloc_objs(VAR_OR_TYPE, COUNT, ...) \
	__alloc_objs(kmalloc, default_gfp(__VA_ARGS__), typeof(VAR_OR_TYPE), COUNT)

/* All kzalloc aliases for kmalloc_(obj|objs). */
#define kzalloc_obj(P, ...) \
	__alloc_objs(kzalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#define kzalloc_objs(P, COUNT, ...) \
	__alloc_objs(kzalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)

/* All kvmalloc aliases for kmalloc_(obj|objs). */
#define kvmalloc_obj(P, ...) \
	__alloc_objs(kvmalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#define kvmalloc_objs(P, COUNT, ...) \
	__alloc_objs(kvmalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)

/* All kvzalloc aliases for kmalloc_(obj|objs). */
#define kvzalloc_obj(P, ...) \
	__alloc_objs(kvzalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#define kvzalloc_objs(P, COUNT, ...) \
	__alloc_objs(kvzalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)

#endif /* _LINUX_SLAB_H */
