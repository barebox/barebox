#ifndef __MACH_OMAP4_GENERIC_H
#define __MACH_OMAP4_GENERIC_H

#include <linux/sizes.h>
#include <mach/generic.h>
#include <mach/omap4-silicon.h>

static inline void omap4_save_bootinfo(uint32_t *info)
{
	unsigned long i = (unsigned long)info;

	if (i & 0x3)
		return;
	if (i < OMAP44XX_SRAM_BASE)
		return;
	if (i > OMAP44XX_SRAM_BASE + SZ_64K)
		return;

	memcpy((void *)OMAP44XX_SRAM_SCRATCH_SPACE, info, 3 * sizeof(uint32_t));
}

int omap4_init(void);
int omap4_devices_init(void);

#endif /* __MACH_OMAP4_GENERIC_H */
