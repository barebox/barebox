/*
 * Copyright (C) 2017 Michael Grzeschik, Pengutronix
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

#include <asm/armlinux.h>
#include <asm/io.h>
#include <bootsource.h>
#include <common.h>
#include <environment.h>
#include <envfs.h>
#include <gpio.h>
#include <init.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <mach/imx6.h>
#include <mach/bbu.h>
#include <linux/sizes.h>
#include <linux/phy.h>
#include <mfd/imx6q-iomuxc-gpr.h>

#include <linux/micrel_phy.h>

static int ksz8081_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x1f, 0x8190);
	phy_write(dev, 0x16, 0x202);

	return 0;
}

static int pico_hobbit_device_init(void)
{
	if (!of_machine_is_compatible("technexion,imx6ul-pico-hobbit"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_KSZ8081, MICREL_PHY_ID_MASK,
			ksz8081_phy_fixup);

	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc0.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("pico-hobbit");

	return 0;
}
device_initcall(pico_hobbit_device_init);
