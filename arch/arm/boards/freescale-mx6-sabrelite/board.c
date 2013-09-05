/*
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
 *
 * based on arch/arm/boards/freescale-mx6-arm2/board.c
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
#include <linux/micrel_phy.h>
#include <mach/imx6.h>
#include <mach/devices-imx6.h>
#include <mach/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/spi.h>
#include <mach/usb.h>

static iomux_v3_cfg_t sabrelite_enet_gpio_pads[] = {
	/* Ethernet */
	MX6Q_PAD_EIM_D23__GPIO_3_23,		/* RGMII_nRST */
	MX6Q_PAD_RGMII_RXC__GPIO_6_30,		/* PHYAD */
	MX6Q_PAD_RGMII_RD0__GPIO_6_25,		/* MODE0 */
	MX6Q_PAD_RGMII_RD1__GPIO_6_27,		/* MODE1 */
	MX6Q_PAD_RGMII_RD2__GPIO_6_28,		/* MODE2 */
	MX6Q_PAD_RGMII_RD3__GPIO_6_29,		/* MODE3 */
	MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24,
};

static int sabrelite_mem_init(void)
{
	arm_add_mem_device("ram0", 0x10000000, SZ_1G);

	return 0;
}
mem_initcall(sabrelite_mem_init);

static int ksz9021rn_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x09, 0x0f00);

	/* do same as linux kernel */
	/* min rx data delay */
	phy_write(dev, 0x0b, 0x8105);
	phy_write(dev, 0x0c, 0x0000);

	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(dev, 0x0b, 0x8104);
	phy_write(dev, 0x0c, 0xf0f0);
	phy_write(dev, 0x0b, 0x104);

	return 0;
}

static int sabrelite_ksz9021rn_setup(void)
{
	mxc_iomux_v3_setup_multiple_pads(sabrelite_enet_gpio_pads,
			ARRAY_SIZE(sabrelite_enet_gpio_pads));

	gpio_direction_output(87, 0);  /* GPIO 3-23 */

	gpio_direction_output(190, 1); /* GPIO 6-30: PHYAD2 */

	/* LED-Mode: Tri-Color Dual LED Mode */
	gpio_direction_output(23 , 0); /* GPIO 1-23 */

	/* MODE strap-in pins: advertise all capabilities */
	gpio_direction_output(185, 1); /* GPIO 6-25 */
	gpio_direction_output(187, 1); /* GPIO 6-27 */
	gpio_direction_output(188, 1); /* GPIO 6-28*/
	gpio_direction_output(189, 1); /* GPIO 6-29 */

	/* Enable 125 MHz clock output */
        gpio_direction_output(184, 1); /* GPIO 6-24 */

	mdelay(10);
	gpio_set_value(87, 1);

	return 0;
}
/*
 * Do this before the fec initializes but after our
 * gpios are available.
 */
fs_initcall(sabrelite_ksz9021rn_setup);

static void sabrelite_ehci_init(void)
{
	imx6_usb_phy2_disable_oc();
	imx6_usb_phy2_enable();

	/* hub reset */
	gpio_direction_output(204, 0);
	udelay(2000);
	gpio_set_value(204, 1);

	add_generic_usb_ehci_device(1, MX6_USBOH3_USB_BASE_ADDR + 0x200, NULL);
}

static int sabrelite_devices_init(void)
{
	sabrelite_ehci_init();

	armlinux_set_bootparams((void *)0x10000100);
	armlinux_set_architecture(3769);

	devfs_add_partition("m25p0", 0, SZ_512K, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("m25p0", SZ_512K, SZ_512K, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
device_initcall(sabrelite_devices_init);

static int sabrelite_coredevices_init(void)
{
	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);
	return 0;
}
coredevice_initcall(sabrelite_coredevices_init);

static int sabrelite_core_init(void)
{
	imx6_init_lowlevel();

	barebox_set_hostname("sabrelite");

	return 0;
}
core_initcall(sabrelite_core_init);
