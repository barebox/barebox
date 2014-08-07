/*
 * Copyright (C) 2014 Andrey Panov <rockford@yandex.ru>
 *
 * based on arch/arm/boards/freescale-mx6-sabresd/board.c
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
#include <envfs.h>

#define PHY_ID_RTL8211E	0x001cc915
#define PHY_ID_MASK	0xffffffff

/*
 * This should reset a PHY. Taken from E9 U-Boot/Linux source.
 */
static int rtl8211e_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x00, 0x3140);
	mdelay(10);
	phy_write(dev, 0x00, 0x3340);
	mdelay(10);

	return 0;
}

static int e9_devices_init(void)
{
	if (!of_machine_is_compatible("embedsky,e9"))
		return 0;

	armlinux_set_architecture(3980);
	barebox_set_hostname("e9");
	defaultenv_append_directory(defaultenv_e9);

	return 0;
}
device_initcall(e9_devices_init);

static int e9_coredevices_init(void)
{
	if (!of_machine_is_compatible("embedsky,e9"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_RTL8211E, PHY_ID_MASK,
			rtl8211e_phy_fixup);

	return 0;
}
/*
 * Do this before the fec initializes but after our
 * gpios are available.
 */
coredevice_initcall(e9_coredevices_init);
