/*
 * Copyright
 * (C) 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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
#include <mach/dove-regs.h>

static inline void dove_remap_mc_regs(void)
{
	void __iomem *mcboot = IOMEM(DOVE_BOOTUP_MC_REGS);
	uint32_t val;

	/* remap ahb slave base */
	val  = readl(DOVE_CPU_CTRL) & 0xffff0000;
	val |= (DOVE_REMAP_MC_REGS & 0xffff0000) >> 16;
	writel(val, DOVE_CPU_CTRL);

	/* remap axi bridge address */
	val  = readl(DOVE_AXI_CTRL) & 0x007fffff;
	val |= DOVE_REMAP_MC_REGS & 0xff800000;
	writel(val, DOVE_AXI_CTRL);

	/* remap memory controller base address */
	val = readl(mcboot + SDRAM_REGS_BASE_DECODE) & 0x0000ffff;
	val |= DOVE_REMAP_MC_REGS & 0xffff0000;
	writel(val, mcboot + SDRAM_REGS_BASE_DECODE);
}

static inline void dove_memory_find(unsigned long *phys_base,
				    unsigned long *phys_size)
{
	int n;

	*phys_base = ~0;
	*phys_size = 0;

	for (n = 0; n < 2; n++) {
		uint32_t map = readl(DOVE_SDRAM_BASE + SDRAM_MAPn(n));
		uint32_t base, size;

		/* skip disabled areas */
		if ((map & SDRAM_MAP_VALID) != SDRAM_MAP_VALID)
			continue;

		base = map & SDRAM_START_MASK;
		if (base < *phys_base)
			*phys_base = base;

		/* real size is encoded as ld(2^(16+length)) */
		size = (map & SDRAM_LENGTH_MASK) >> SDRAM_LENGTH_SHIFT;
		*phys_size += 1 << (16 + size);
	}
}

static void __noreturn dove_reset_cpu(unsigned long addr)
{
	/* enable and assert RSTOUTn */
	writel(SOFT_RESET_OUT_EN, DOVE_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, DOVE_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);
	while (1)
		;
}

static int dove_init_soc(struct device_node *root, void *context)
{
	unsigned long phys_base, phys_size;

	if (!of_machine_is_compatible("marvell,dove"))
		return 0;

	mvebu_set_reset(dove_reset_cpu);

	barebox_set_model("Marvell Dove");
	barebox_set_hostname("dove");

	dove_remap_mc_regs();
	dove_memory_find(&phys_base, &phys_size);

	mvebu_set_memory(phys_base, phys_size);

	return 0;
}

static int dove_register_soc_fixup(void)
{
	mvebu_mbus_add_range("marvell,dove", 0xf0, 0x01,
			     MVEBU_REMAP_INT_REG_BASE);
	mvebu_mbus_add_range("marvell,dove", 0xf0, 0x02,
			     DOVE_REMAP_MC_REGS);
	return of_register_fixup(dove_init_soc, NULL);
}
pure_initcall(dove_register_soc_fixup);
