/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARM_MMU_COMMON_H
#define __ARM_MMU_COMMON_H

#include <printk.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

struct device;

void dma_inv_range(void *ptr, size_t size);
void dma_flush_range(void *ptr, size_t size);
void *dma_alloc_map(struct device *dev, size_t size, dma_addr_t *dma_handle, unsigned flags);
void __mmu_init(bool mmu_on);

static inline void arm_mmu_not_initialized_error(void)
{
	/*
	 * This means:
	 * - one of the MMU functions like dma_alloc_coherent
	 *   or remap_range is called too early, before the MMU is initialized
	 * - Or the MMU initialization has failed earlier
	 */
	panic("MMU not initialized\n");
}

static inline size_t resource_first_page(const struct resource *res)
{
	return ALIGN_DOWN(res->start, SZ_4K);
}

static inline size_t resource_count_pages(const struct resource *res)
{
	return ALIGN(resource_size(res), SZ_4K);
}

#endif
