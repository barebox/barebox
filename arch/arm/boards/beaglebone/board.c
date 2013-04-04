/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
 *
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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

/**
 * @file
 * @brief BeagleBone Specific Board Initialization routines
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <envfs.h>
#include <sizes.h>
#include <io.h>
#include <ns16550.h>
#include <net.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/gpmc.h>
#include <mach/ehci.h>
#include <i2c/i2c.h>
#include <linux/err.h>
#include <linux/phy.h>
#include <usb/ehci.h>
#include <mach/am33xx-devices.h>
#include <mach/am33xx-mux.h>
#include <mach/wdt.h>
#include <mach/am33xx-generic.h>
#include <mach/cpsw.h>

#ifdef CONFIG_DRIVER_SERIAL_NS16550

/**
 * @brief UART serial port initialization - remember to enable COM clocks in
 * arch
 *
 * @return result of device registration
 */
static int beaglebone_console_init(void)
{
	am33xx_add_uart0();

	return 0;
}
console_initcall(beaglebone_console_init);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

static int beaglebone_mem_init(void)
{
	omap_add_ram0(SZ_256M);

	return 0;
}
mem_initcall(beaglebone_mem_init);

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.phy_id		= 0,
		.phy_if		= PHY_INTERFACE_MODE_MII,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.slave_data		= cpsw_slaves,
	.num_slaves		= ARRAY_SIZE(cpsw_slaves),
};

static void beaglebone_eth_init(void)
{
	am33xx_register_ethaddr(0, 0);

	writel(0, AM33XX_MAC_MII_SEL);

	am33xx_enable_mii1_pin_mux();

	am33xx_add_cpsw(&cpsw_data);
}

static int beaglebone_devices_init(void)
{
	am33xx_add_mmc0(NULL);

	am33xx_enable_i2c0_pin_mux();
	beaglebone_eth_init();

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_BEAGLEBONE);

	return 0;
}
device_initcall(beaglebone_devices_init);
