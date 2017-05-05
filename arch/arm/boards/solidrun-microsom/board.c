/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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
#include <linux/clk.h>
#include <linux/sizes.h>
#include <linux/phy.h>

static int ar8035_phy_fixup(struct phy_device *dev)
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

static void microsom_eth_init(void)
{
	void __iomem *iomux = (void *)MX6_IOMUXC_BASE_ADDR;
	u32 val;

	clk_set_rate(clk_lookup("enet_ref"), 25000000);

	val = readl(iomux + IOMUXC_GPR1);
	val |= IMX6Q_GPR1_ENET_CLK_SEL_ANATOP;
	writel(val, iomux + IOMUXC_GPR1);

	phy_register_fixup_for_uid(0x004dd072, 0xffffffef, ar8035_phy_fixup);
}
static int hummingboard_device_init(void)
{
	if (!of_machine_is_compatible("solidrun,hummingboard/dl") &&
	    !of_machine_is_compatible("solidrun,hummingboard/q") &&
	    !of_machine_is_compatible("solidrun,hummingboard2/dl") &&
	    !of_machine_is_compatible("solidrun,hummingboard2/q"))
		return 0;

	microsom_eth_init();

	/* enable USB VBUS */
	gpio_direction_output(IMX_GPIO_NR(3, 22), 1);
	gpio_direction_output(IMX_GPIO_NR(1, 0), 1);

	barebox_set_hostname("hummingboard");

	return 0;
}
device_initcall(hummingboard_device_init);

static int h100_device_init(void)
{
	if (!of_machine_is_compatible("auvidea,h100"))
		return 0;

	microsom_eth_init();

	barebox_set_hostname("h100");

	return 0;
}
device_initcall(h100_device_init);

static int hummingboard_late_init(void)
{
	bool emmc_present = false;

	if (!of_machine_is_compatible("solidrun,hummingboard/dl") &&
	    !of_machine_is_compatible("solidrun,hummingboard/q") &&
	    !of_machine_is_compatible("solidrun,hummingboard2/dl") &&
	    !of_machine_is_compatible("solidrun,hummingboard2/q") &&
	    !of_machine_is_compatible("auvidea,h100"))
		return 0;

	if (of_machine_is_compatible("solidrun,hummingboard2/dl") ||
	    of_machine_is_compatible("solidrun,hummingboard2/q"))
		emmc_present = true;

	imx6_bbu_internal_mmc_register_handler("sdcard", "/dev/mmc1.barebox",
		emmc_present ? 0 : BBU_HANDLER_FLAG_DEFAULT);

	if (emmc_present) {
		imx6_bbu_internal_mmc_register_handler("emmc",
			"/dev/mmc2.barebox", BBU_HANDLER_FLAG_DEFAULT);
	}

	return 0;
}
late_initcall(hummingboard_late_init);
