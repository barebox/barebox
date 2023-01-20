/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Basic general purpose allocator for managing special purpose
 * memory, for example, memory that is not managed by the regular
 * kmalloc/kfree interface.  Uses for this includes on-device special
 * memory, uncached memory etc.
 */


#ifndef __GENALLOC_H__
#define __GENALLOC_H__

#include <linux/types.h>

struct device_node;

struct gen_pool;

extern phys_addr_t gen_pool_virt_to_phys(struct gen_pool *pool, unsigned long);

extern void *gen_pool_dma_alloc(struct gen_pool *pool, size_t size,
		dma_addr_t *dma);

extern void *gen_pool_dma_zalloc(struct gen_pool *pool, size_t size, dma_addr_t *dma);

#ifdef CONFIG_OFDEVICE
extern struct gen_pool *of_gen_pool_get(struct device_node *np,
	const char *propname, int index);
#else
static inline struct gen_pool *of_gen_pool_get(struct device_node *np,
	const char *propname, int index)
{
	return NULL;
}
#endif
#endif /* __GENALLOC_H__ */
