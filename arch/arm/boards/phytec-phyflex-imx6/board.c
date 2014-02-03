/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <of.h>

#include <mach/imx6.h>

#define ETH_PHY_RST	IMX_GPIO_NR(3, 23)

static int eth_phy_reset(void)
{
	gpio_request(ETH_PHY_RST, "phy reset");
	gpio_direction_output(ETH_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(ETH_PHY_RST, 1);

	return 0;
}

static int phytec_pfla02_init(void)
{
	if (!of_machine_is_compatible("phytec,imx6q-pfla02"))
		return 0;

	eth_phy_reset();

	return 0;
}
device_initcall(phytec_pfla02_init);

static int phytec_pfla02_core_init(void)
{
	if (!of_machine_is_compatible("phytec,imx6q-pfla02"))
		return 0;

	imx6_init_lowlevel();

	return 0;
}
postcore_initcall(phytec_pfla02_core_init);
