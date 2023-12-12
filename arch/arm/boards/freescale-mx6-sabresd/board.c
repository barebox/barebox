// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2013 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on arch/arm/boards/freescale-mx6-sabrelite/board.c
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/imx6-regs.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <linux/phy.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/imx/generic.h>
#include <linux/sizes.h>
#include <net.h>
#include <mach/imx/imx6.h>
#include <mach/imx/devices-imx6.h>
#include <mach/imx/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/imx/spi.h>
#include <mach/imx/usb.h>

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
	if (!of_machine_is_compatible("fsl,imx6q-sabresd") &&
	    !of_machine_is_compatible("fsl,imx6qp-sabresd"))
		return 0;

	armlinux_set_architecture(3980);
	barebox_set_hostname("sabresd");

	return 0;
}
device_initcall(sabresd_devices_init);

static int sabresd_coredevices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6q-sabresd") &&
	    !of_machine_is_compatible("fsl,imx6qp-sabresd"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
			ar8031_phy_fixup);

	return 0;
}
coredevice_initcall(sabresd_coredevices_init);
