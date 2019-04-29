// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright
 * (C) 2019 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>

static int pinctrl_init(void)
{
	if (!of_machine_is_compatible("marvell,axp-db"))
		return 0;

	/* IOMUX setup for the poor */
	writel(0x11111111, 0xf1018000);
	writel(0x22221111, 0xf1018004);
	writel(0x22222222, 0xf1018008);
	writel(0x11040000, 0xf101800c);
	writel(0x11111111, 0xf1018010);
	writel(0x04221130, 0xf1018014);
	writel(0x11111113, 0xf1018018);
	writel(0x11111101, 0xf101801c);

	writel(0x7fffffe1, 0xf1020184);
	writel(0x00010008, 0xf1020258);

	return 0;
}
coredevice_initcall(pinctrl_init);
