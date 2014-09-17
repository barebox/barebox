/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <mach/kirkwood-regs.h>

static inline void kirkwood_memory_find(unsigned long *phys_base,
					unsigned long *phys_size)
{
	int cs;

	*phys_base = ~0;
	*phys_size = 0;

	for (cs = 0; cs < 4; cs++) {
		u32 base = readl(KIRKWOOD_SDRAM_BASE + DDR_BASE_CSn(cs));
		u32 ctrl = readl(KIRKWOOD_SDRAM_BASE + DDR_SIZE_CSn(cs));

		/* Skip non-enabled CS */
		if ((ctrl & DDR_SIZE_ENABLED) != DDR_SIZE_ENABLED)
			continue;

		base &= DDR_BASE_CS_LOW_MASK;
		if (base < *phys_base)
			*phys_base = base;
		*phys_size += (ctrl | ~DDR_SIZE_MASK) + 1;
	}
}

static void __noreturn kirkwood_reset_cpu(unsigned long addr)
{
	writel(SOFT_RESET_OUT_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);
	for(;;)
		;
}

static int kirkwood_init_soc(struct device_node *root, void *context)
{
	unsigned long phys_base, phys_size;

	if (!of_machine_is_compatible("marvell,kirkwood"))
		return 0;

	mvebu_set_reset(kirkwood_reset_cpu);

	barebox_set_model("Marvell Kirkwood");
	barebox_set_hostname("kirkwood");

	kirkwood_memory_find(&phys_base, &phys_size);

	mvebu_set_memory(phys_base, phys_size);

	return 0;
}

static int kirkwood_register_soc_fixup(void)
{
	mvebu_mbus_add_range("marvell,kirkwood", 0xf0, 0x01,
			     MVEBU_REMAP_INT_REG_BASE);
	return of_register_fixup(kirkwood_init_soc, NULL);
}
pure_initcall(kirkwood_register_soc_fixup);
