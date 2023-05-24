/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MMU_H
#define __MMU_H

#include <linux/types.h>
#include <errno.h>

#define MAP_UNCACHED	0
#define MAP_CACHED	1
#define MAP_FAULT	2

/*
 * Depending on the architecture the default mapping can be
 * cached or uncached. Without ARCH_HAS_REMAP being set this
 * is mapping type is the only one supported.
 */
#define MAP_DEFAULT	MAP_ARCH_DEFAULT

#include <asm/mmu.h>

#ifndef ARCH_HAS_REMAP
static inline int arch_remap_range(void *virt_addr, phys_addr_t phys_addr,
				   size_t size, unsigned flags)
{
	if (flags == MAP_ARCH_DEFAULT && phys_addr == virt_to_phys(virt_addr))
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

static inline int remap_range(void *start, size_t size, unsigned flags)
{
	return arch_remap_range(start, virt_to_phys(start), size, flags);
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
