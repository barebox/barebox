/*
 * Copyright (C) 2017 Sascha Hauer, Pengutronix
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
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/bbu.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <mach/generic.h>
#include <linux/sizes.h>

static int warp7_devices_init(void)
{
	if (!of_machine_is_compatible("warp,imx7s-warp"))
		return 0;

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc2.boot0.barebox",
					       BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
device_initcall(warp7_devices_init);
