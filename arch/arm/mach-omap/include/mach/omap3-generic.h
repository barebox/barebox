#ifndef __MACH_OMAP3_GENERIC_H
#define __MACH_OMAP3_GENERIC_H

#include <sizes.h>
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

u32 omap3_running_in_flash(void);
u32 omap3_running_in_sram(void);
u32 omap3_running_in_sdram(void);

void __noreturn omap3_reset_cpu(unsigned long addr);

int omap3_init(void);
int omap3_devices_init(void);

#endif /* __MACH_OMAP3_GENERIC_H */
