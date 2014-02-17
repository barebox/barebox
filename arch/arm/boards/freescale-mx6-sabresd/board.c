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

static int sabresd_devices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6q-sabresd"))
		return 0;

	armlinux_set_architecture(3980);

	return 0;
}
device_initcall(sabresd_devices_init);

static int sabresd_coredevices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6q-sabresd"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
			ar8031_phy_fixup);

	return 0;
}
/*
 * Do this before the fec initializes but after our
 * gpios are available.
 */
coredevice_initcall(sabresd_coredevices_init);

static int sabresd_postcore_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6q-sabresd"))
		return 0;

	imx6_init_lowlevel();

	barebox_set_hostname("sabresd");

	return 0;
}
postcore_initcall(sabresd_postcore_init);
