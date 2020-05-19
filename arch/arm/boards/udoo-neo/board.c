// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Sascha Hauer, Pengutronix

#include <common.h>
#include <init.h>
#include <linux/clk.h>

static int imx6sx_udoneo_coredevices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6sx-udoo-neo"))
		return 0;

	barebox_set_hostname("mx6sx-udooneo");

	return 0;
}
coredevice_initcall(imx6sx_udoneo_coredevices_init);
