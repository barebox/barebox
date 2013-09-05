#ifndef __MACH_AM33XX_GENERIC_H
#define __MACH_AM33XX_GENERIC_H

#include <mach/generic.h>
#include <mach/am33xx-silicon.h>

int am33xx_register_ethaddr(int eth_id, int mac_id);

u32 am33xx_get_cpu_rev(void);

static inline void am33xx_save_bootinfo(uint32_t *info)
{
	unsigned long i = (unsigned long)info;

	if (i & 0x3)
		return;
	if (i < AM33XX_SRAM0_START)
		return;
	if (i > AM33XX_SRAM0_START + AM33XX_SRAM0_SIZE)
		return;

	omap_save_bootinfo(info);
}

#endif /* __MACH_AM33XX_GENERIC_H */
