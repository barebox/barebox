/*
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <asm/system_info.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/pgtable.h>
#include <asm/cache.h>

/*
 * relocate binary to the currently running address
 */
void relocate_to_current_adr(void)
{
	uint32_t offset;
	uint32_t *dstart, *dend, *dynsym, *dynend;

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

	dstart = (void *)(ld_var(__rel_dyn_start) + offset);
	dend = (void *)(ld_var(__rel_dyn_end) + offset);

	dynsym = (void *)(ld_var(__dynsym_start) + offset);
	dynend = (void *)(ld_var(__dynsym_end) + offset);

	while (dstart < dend) {
		uint32_t *fixup = (uint32_t *)(*dstart + offset);
		uint32_t type = *(dstart + 1);

		if ((type & 0xff) == 0x17) {
			*fixup = *fixup + offset;
		} else {
			int index = type >> 8;
			uint32_t r = dynsym[index * 4 + 1];

			*fixup = *fixup + r + offset;
		}

		*dstart += offset;
		dstart += 2;
	}

	memset(dynsym, 0, (unsigned long)dynend - (unsigned long)dynsym);

	arm_early_mmu_cache_flush();
	icache_invalidate();
}

#ifdef ARM_MULTIARCH

int __cpu_architecture;

int __pure cpu_architecture(void)
{
	if(__cpu_architecture == CPU_ARCH_UNKNOWN)
		__cpu_architecture = arm_early_get_cpu_architecture();

	return __cpu_architecture;
}
#endif

char __image_start[0] __attribute__((section(".__image_start")));
char __image_end[0] __attribute__((section(".__image_end")));