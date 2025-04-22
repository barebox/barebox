/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_AM33XX_GENERIC_H
#define __MACH_AM33XX_GENERIC_H

#include <string.h>
#include <mach/omap/generic.h>
#include <mach/omap/am33xx-silicon.h>

int am33xx_register_ethaddr(int eth_id, int mac_id);

u32 am33xx_get_cpu_rev(void);

static inline void am33xx_save_bootinfo(uint32_t *info)
{
	unsigned long i = (unsigned long)info;
	uint32_t *scratch = (void *)AM33XX_SRAM_SCRATCH_SPACE;

	if (i & 0x3)
		return;
	if (i < AM33XX_SRAM0_START)
		return;
	if (i > AM33XX_SRAM0_START + AM33XX_SRAM0_SIZE)
		return;

	memcpy(scratch, info, 3 * sizeof(uint32_t));
}

u32 am33xx_running_in_flash(void);
u32 am33xx_running_in_sram(void);
u32 am33xx_running_in_sdram(void);

enum bootsource am33xx_get_bootsource(int *instance);

void am33xx_enable_per_clocks(void);
int am33xx_init(void);
int am33xx_devices_init(void);
void am33xx_select_rmii2_crs_dv(void);
int am33xx_of_register_bootdevice(void);

#endif /* __MACH_AM33XX_GENERIC_H */
