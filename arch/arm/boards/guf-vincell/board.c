// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 Sascha Hauer, Pengutronix
// SPDX-FileCopyrightText: 2011 Marc Kleine-Budde <mkl@pengutronix.de>

#include <common.h>
#include <environment.h>
#include <init.h>
#include <linux/sizes.h>
#include <bbu.h>
#include <gpio.h>
#include <io.h>
#include <linux/clk.h>

#include <mach/imx/generic.h>
#include <mach/imx/iim.h>
#include <mach/imx/bbu.h>
#include <mach/imx/imx5.h>
#include <mach/imx/imx53-regs.h>

static int vincell_devices_init(void)
{
	if (!of_machine_is_compatible("guf,imx53-vincell") &&
	    !of_machine_is_compatible("guf,imx53-vincell-lt"))
		return 0;

	writel(0, MX53_M4IF_BASE_ADDR + 0xc);

	clk_set_rate(clk_lookup("emi_slow_podf"), 133333334);
	clk_set_rate(clk_lookup("nfc_podf"), 33333334);

	imx53_bbu_internal_nand_register_handler("nand",
		BBU_HANDLER_FLAG_DEFAULT, 0xe0000);

	return 0;
}

coredevice_initcall(vincell_devices_init);
