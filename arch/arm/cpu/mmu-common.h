/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARM_MMU_COMMON_H
#define __ARM_MMU_COMMON_H

#include <mmu.h>
#include <printf.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/bits.h>

#define ARCH_MAP_CACHED_RWX	MAP_ARCH(2)
#define ARCH_MAP_CACHED_RO	MAP_ARCH(3)

#define ARCH_MAP_FLAG_PAGEWISE	BIT(31)

struct device;

void dma_inv_range(void *ptr, size_t size);
void dma_flush_range(void *ptr, size_t size);
void *dma_alloc_map(struct device *dev, size_t size, dma_addr_t *dma_handle, maptype_t map_type);
void setup_trap_pages(void);
void __mmu_init(bool mmu_on);

static inline maptype_t arm_mmu_maybe_skip_permissions(maptype_t map_type)
{
	if (IS_ENABLED(CONFIG_ARM_MMU_PERMISSIONS))
		return map_type;

	switch (map_type & MAP_TYPE_MASK) {
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

const char *map_type_tostr(maptype_t map_type);

static inline void __pr_debug_remap(const char *func, ulong virt_addr, ulong phys_addr,
				  size_t size, maptype_t map_type)
{
	if (phys_addr == virt_addr)
		pr_debug("%s: 0x%08lx+0x%zx type %s\n",	func,
			 virt_addr, size, map_type_tostr(map_type));
	else
		pr_debug("%s: 0x%08lx+0x%zx -> 0x%08lx type %s\n", func,
			 virt_addr, size, phys_addr, map_type_tostr(map_type));
}

#define pr_debug_remap(virt_addr, phys_addr, size, map_type)	\
	__pr_debug_remap(__func__, virt_addr, phys_addr, size, map_type)

#endif
