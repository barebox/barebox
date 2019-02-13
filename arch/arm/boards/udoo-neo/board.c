/*
 * Copyright (C) 2014 Pengutronix, Sascha Hauer
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
