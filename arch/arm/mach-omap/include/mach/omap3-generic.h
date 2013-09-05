#ifndef __MACH_OMAP3_GENERIC_H
#define __MACH_OMAP3_GENERIC_H

#include <mach/generic.h>
#include <mach/omap3-silicon.h>

static inline void omap3_save_bootinfo(uint32_t *info)
{
	unsigned long i = (unsigned long)info;

	if (i & 0x3)
		return;
	if (i < OMAP3_SRAM_BASE)
		return;
	if (i > OMAP3_SRAM_BASE + SZ_64K)
		return;

	omap_save_bootinfo(info);
}

#endif /* __MACH_OMAP3_GENERIC_H */
