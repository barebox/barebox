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
#include <restart.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <mach/kirkwood-regs.h>

static void __noreturn kirkwood_restart_soc(struct restart_handler *rst)
{
	writel(SOFT_RESET_OUT_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);

	hang();
}

static int kirkwood_init_soc(void)
{
	if (!of_machine_is_compatible("marvell,kirkwood"))
		return 0;

	restart_handler_register_fn(kirkwood_restart_soc);

	barebox_set_model("Marvell Kirkwood");
	barebox_set_hostname("kirkwood");

	mvebu_mbus_init();

	return 0;
}
postcore_initcall(kirkwood_init_soc);
