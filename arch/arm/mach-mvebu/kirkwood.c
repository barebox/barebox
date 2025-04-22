// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <mach/mvebu/kirkwood-regs.h>

static void __noreturn kirkwood_restart_soc(struct restart_handler *rst,
					    unsigned long flags)
{
	writel(SOFT_RESET_OUT_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, KIRKWOOD_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);

	hang();
}

static int kirkwood_init_soc(void)
{
	if (!of_machine_is_compatible("marvell,kirkwood"))
		return 0;

	restart_handler_register_fn("soc", kirkwood_restart_soc);

	barebox_set_model("Marvell Kirkwood");
	barebox_set_hostname("kirkwood");

	mvebu_mbus_init();

	return 0;
}
postcore_initcall(kirkwood_init_soc);
