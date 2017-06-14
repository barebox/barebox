/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <asm/armlinux.h>
#include <asm/io.h>
#include <bootsource.h>
#include <common.h>
#include <environment.h>
#include <envfs.h>
#include <gpio.h>
#include <init.h>
#include <mach/bbu.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <mach/imx6.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <linux/sizes.h>
#include <linux/phy.h>

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

static int ar8031_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* Ar803x phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	phy_write(dev, 0xd, 0x3);
	phy_write(dev, 0xe, 0x805d);
	phy_write(dev, 0xd, 0x4003);

	val = phy_read(dev, 0xe);
	phy_write(dev, 0xe, val & ~(1 << 8));

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(dev, 0xd, 0x7);
	phy_write(dev, 0xe, 0x8016);
	phy_write(dev, 0xd, 0x4007);

	val = phy_read(dev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(dev, 0x1d, 0x5);
	val = phy_read(dev, 0x1e);
	val |= 0x0100;
	phy_write(dev, 0x1e, val);

	return 0;
}

static int wandboard_device_init(void)
{
	if (!of_machine_is_compatible("wand,imx6dl-wandboard") &&
	    !of_machine_is_compatible("wand,imx6q-wandboard"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK, ar8031_phy_fixup);

	barebox_set_hostname("wandboard");

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc2.barebox",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
device_initcall(wandboard_device_init);
