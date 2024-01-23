// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>

#include <asm/armlinux.h>
#include <common.h>
#include <environment.h>
#include <asm/mach-types.h>
#include <init.h>
#include <mach/zynq/zynq7000-regs.h>
#include <linux/sizes.h>


static int zedboard_console_init(void)
{
	barebox_set_hostname("zedboard");

	return 0;
}
console_initcall(zedboard_console_init);
