/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARM_MMU_COMMON_H
#define __ARM_MMU_COMMON_H

#include <mmu.h>
#include <printf.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

#define ARCH_MAP_WRITECOMBINE	((unsigned)-1)
#define ARCH_MAP_CACHED_RWX	((unsigned)-2)
#define ARCH_MAP_CACHED_RO	((unsigned)-3)

struct device;

void dma_inv_range(void *ptr, size_t size);
void dma_flush_range(void *ptr, size_t size);
void *dma_alloc_map(struct device *dev, size_t size, dma_addr_t *dma_handle, unsigned flags);
void setup_trap_pages(void);
void __mmu_init(bool mmu_on);

static inline unsigned arm_mmu_maybe_skip_permissions(unsigned map_type)
{
	if (IS_ENABLED(CONFIG_ARM_MMU_PERMISSIONS))
		return map_type;

	switch (map_type) {
	case MAP_CODE:
	case MAP_CACHED:
	case ARCH_MAP_CACHED_RO:
		return ARCH_MAP_CACHED_RWX;
	default:
		return map_type;
	}
}

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
