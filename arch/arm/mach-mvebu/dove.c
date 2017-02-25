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
#include <restart.h>
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

static void __noreturn dove_restart_soc(struct restart_handler *rst)
{
	/* enable and assert RSTOUTn */
	writel(SOFT_RESET_OUT_EN, DOVE_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, DOVE_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);

	hang();
}

static int dove_init_soc(void)
{
	if (!of_machine_is_compatible("marvell,dove"))
		return 0;

	restart_handler_register_fn(dove_restart_soc);

	barebox_set_model("Marvell Dove");
	barebox_set_hostname("dove");

	dove_remap_mc_regs();
	mvebu_mbus_init();

	return 0;
}
postcore_initcall(dove_init_soc);
