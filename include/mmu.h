#ifndef __MMU_H
#define __MMU_H

#define MAP_UNCACHED	0
#define MAP_CACHED	1

/*
 * Depending on the architecture the default mapping can be
 * cached or uncached. Without ARCH_HAS_REMAP being set this
 * is mapping type is the only one supported.
 */
#define MAP_DEFAULT	MAP_ARCH_DEFAULT

#include <asm/mmu.h>

#ifndef ARCH_HAS_REMAP
static inline int arch_remap_range(void *start, size_t size, unsigned flags)
{
	if (flags == MAP_ARCH_DEFAULT)
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
	return arch_remap_range(start, size, flags);
}

#endif
