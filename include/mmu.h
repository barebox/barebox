/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MMU_H
#define __MMU_H

#include <linux/types.h>
#include <errno.h>

#define MAP_UNCACHED		0
#define MAP_CACHED		1
#define MAP_FAULT		2
#define MAP_CODE		3

#ifdef CONFIG_ARCH_HAS_DMA_WRITE_COMBINE
#define MAP_WRITECOMBINE	4
#else
#define MAP_WRITECOMBINE	MAP_UNCACHED
#endif

#define MAP_TYPE_MASK	0xFFFF
#define MAP_ARCH(x)	((u16)~(x))

/*
 * Depending on the architecture the default mapping can be
 * cached or uncached. Without ARCH_HAS_REMAP being set this
 * is mapping type is the only one supported.
 */
#define MAP_DEFAULT	MAP_ARCH_DEFAULT

#include <asm/mmu.h>

static inline bool maptype_is_compatible(maptype_t active, maptype_t check)
{
	active &= MAP_TYPE_MASK;
	check &= MAP_TYPE_MASK;

	if (active == check)
		return true;
	if (active == MAP_CODE && check == MAP_CACHED)
		return true;

	return false;
}

#ifndef ARCH_HAS_REMAP
static inline int arch_remap_range(void *virt_addr, phys_addr_t phys_addr,
				   size_t size, maptype_t map_type)
{
	if (maptype_is_compatible(map_type, MAP_ARCH_DEFAULT) &&
	    phys_addr == virt_to_phys(virt_addr))
		return 0;

	return -EINVAL;
}

static inline bool arch_can_remap(void)
{
	return false;
}
#else
static inline bool arch_can_remap(void)
{
	return true;
}
#endif

static inline int remap_range(void *start, size_t size, maptype_t map_type)
{
	return arch_remap_range(start, virt_to_phys(start), size, map_type);
}

#ifdef CONFIG_MMUINFO
int mmuinfo(void *addr);
#else
static inline int mmuinfo(void *addr)
{
	return -ENOSYS;
}
#endif

#endif
