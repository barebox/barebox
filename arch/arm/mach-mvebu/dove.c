// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <mach/mvebu/dove-regs.h>

static void __noreturn dove_restart_soc(struct restart_handler *rst,
					unsigned long flags)
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

	restart_handler_register_fn("soc", dove_restart_soc);

	barebox_set_model("Marvell Dove");
	barebox_set_hostname("dove");

	mvebu_mbus_init();

	return 0;
}
postcore_initcall(dove_init_soc);
