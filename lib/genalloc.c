// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2005 Jes Sorensen <jes@trained-monkey.org>
/*
 * Basic general purpose allocator for managing special purpose
 * memory, for example, memory that is not managed by the regular
 * kmalloc/kfree interface.  Uses for this includes on-device special
 * memory, uncached memory etc.
 */

#include <io.h>
#include <linux/ioport.h>
#include <linux/genalloc.h>
#include <linux/export.h>
#include <of.h>
#include <of_address.h>
#include <driver.h>
#include <linux/string.h>

struct gen_pool {
	struct resource res;
};

#define res_to_gen_pool(res) \
	container_of(res, struct gen_pool, res)

/**
 * gen_pool_virt_to_phys - return the physical address of memory
 * @pool: pool to allocate from
 * @addr: starting address of memory
 *
 * Returns the physical address on success, or -1 on error.
 */
phys_addr_t gen_pool_virt_to_phys(struct gen_pool *pool, unsigned long addr)
{
	return virt_to_phys((void *)addr);
}
EXPORT_SYMBOL(gen_pool_virt_to_phys);

/**
 * gen_pool_dma_alloc - allocate special memory from the pool for DMA usage
 * @pool: pool to allocate from
 * @size: number of bytes to allocate from the pool
 * @dma: dma-view physical address return value.  Use %NULL if unneeded.
 *
 * Allocate the requested number of bytes from the specified pool.
 * Uses the pool allocation function (with first-fit algorithm by default).
 * Can not be used in NMI handler on architectures without
 * NMI-safe cmpxchg implementation.
 *
 * Return: virtual address of the allocated memory, or %NULL on failure
 */
void *gen_pool_dma_alloc(struct gen_pool *pool, size_t size, dma_addr_t *dma)
{
	unsigned long vaddr;

	if (!pool || resource_size(&pool->res) != size)
		return NULL;

	vaddr = pool->res.start;

	if (dma)
		*dma = gen_pool_virt_to_phys(pool, vaddr);

	return (void *)vaddr;
}
EXPORT_SYMBOL(gen_pool_dma_alloc);

/**
 * gen_pool_dma_zalloc - allocate special zeroed memory from the pool for
 * DMA usage
 * @pool: pool to allocate from
 * @size: number of bytes to allocate from the pool
 * @dma: dma-view physical address return value.  Use %NULL if unneeded.
 *
 * Return: virtual address of the allocated zeroed memory, or %NULL on failure
 */
void *gen_pool_dma_zalloc(struct gen_pool *pool, size_t size, dma_addr_t *dma)
{
	void *vaddr = gen_pool_dma_alloc(pool, size, dma);

	if (vaddr)
		memset(vaddr, 0, size);

	return vaddr;
}
EXPORT_SYMBOL(gen_pool_dma_zalloc);

#ifdef CONFIG_OFDEVICE
/**
 * of_gen_pool_get - find a pool by phandle property
 * @np: device node
 * @propname: property name containing phandle(s)
 * @index: index into the phandle array
 *
 * Returns the pool that contains the chunk starting at the physical
 * address of the device tree node pointed at by the phandle property,
 * or NULL if not found.
 */
struct gen_pool *of_gen_pool_get(struct device_node *np,
	const char *propname, int index)
{
	struct device_node *np_pool;
	struct gen_pool gen_pool;
	int ret;

	np_pool = of_parse_phandle(np, propname, index);
	if (!np_pool)
		return NULL;

	if (!of_device_is_compatible(np_pool, "mmio-sram"))
		return NULL;

	ret = of_address_to_resource(np_pool, 0, &gen_pool.res);
	if (ret)
		return NULL;

	return memdup(&gen_pool, sizeof(gen_pool));
}
EXPORT_SYMBOL_GPL(of_gen_pool_get);
#endif /* CONFIG_OF */
