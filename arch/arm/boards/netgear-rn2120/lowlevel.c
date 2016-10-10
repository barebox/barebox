/*
 * Copyright (C) 2015 Pengutronix, Uwe Kleine-König <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/io.h>
#include <mach/lowlevel.h>

extern char __dtb_armada_xp_rn2120_bb_start[];

ENTRY_FUNCTION(start_netgear_rn2120, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	/*
	 * This is necessary to allow the machine to draw more power. Probably
	 * connected to a TI TPS65251. Without this resetting a phy makes the
	 * SoC reset.
	 * This is effectively gpio_direction_output(42, 1);
	 */
	writel((1 << 10) | readl((void *)0xd0018140), (void *)0xd0018140);
	writel(~(1 << 10) & readl((void *)0xd0018144), (void *)0xd0018144);

	fdt = __dtb_armada_xp_rn2120_bb_start -
		get_runtime_offset();

	mvebu_barebox_entry(fdt);
}
