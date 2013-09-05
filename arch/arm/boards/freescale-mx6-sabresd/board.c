/*
 * Copyright (C) 2013 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on arch/arm/boards/freescale-mx6-sabrelite/board.c
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
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
#include <mach/imx6-regs.h>
#include <fec.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <linux/phy.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <sizes.h>
#include <net.h>
#include <mach/imx6.h>
#include <mach/devices-imx6.h>
#include <mach/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/spi.h>
#include <mach/usb.h>

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

static int sabresd_mem_init(void)
{
	arm_add_mem_device("ram0", 0x10000000, SZ_1G);

	return 0;
}
mem_initcall(sabresd_mem_init);

static int ar8031_phy_fixup(struct phy_device *dev)
{
	u16 val;

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

static void sabresd_phy_reset(void)
{
	/* Reset AR8031 PHY */
	gpio_direction_output(IMX_GPIO_NR(1, 25) , 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);
}

static int sabresd_devices_init(void)
{
	armlinux_set_bootparams((void *)0x10000100);
	armlinux_set_architecture(3980);

	devfs_add_partition("disk0", 0, SZ_1M, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", SZ_1M + SZ_1M, SZ_512K, DEVFS_PARTITION_FIXED, "env0");
	return 0;
}
device_initcall(sabresd_devices_init);

static int sabresd_coredevices_init(void)
{
	sabresd_phy_reset();

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
			ar8031_phy_fixup);

	return 0;
}
/*
 * Do this before the fec initializes but after our
 * gpios are available.
 */
fs_initcall(sabresd_coredevices_init);

static int sabresd_core_init(void)
{
	imx6_init_lowlevel();

	barebox_set_hostname("sabresd");

	return 0;
}
core_initcall(sabresd_core_init);
