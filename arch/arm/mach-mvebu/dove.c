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

	mvebu_mbus_init();

	return 0;
}
postcore_initcall(dove_init_soc);
