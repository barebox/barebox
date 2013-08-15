/*
 * Copyright (C) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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

#include <asm/armlinux.h>
#include <common.h>
#include <environment.h>
#include <generated/mach-types.h>
#include <init.h>
#include <mach/devices.h>
#include <mach/zynq7000-regs.h>
#include <sizes.h>

static int zedboard_mem_init(void)
{
	arm_add_mem_device("ram0", 0, SZ_512M);

	return 0;
}
mem_initcall(zedboard_mem_init);

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RGMII,
	.phy_addr = 0x0,
};

static int zedboard_device_init(void)
{
	zynq_add_eth0(&macb_pdata);

	return 0;
}
device_initcall(zedboard_device_init);

static int zedboard_console_init(void)
{
	barebox_set_model("Avnet ZedBoard");
	barebox_set_hostname("zedboard");

	zynq_add_uart1();

	return 0;
}
console_initcall(zedboard_console_init);
