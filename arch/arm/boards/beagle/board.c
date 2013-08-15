/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
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
 */

/**
 * @file
 * @brief Beagle Specific Board Initialization routines
 */

/**
 * @page ti_beagle Texas Instruments Beagle Board
 *
 * Beagle Board from Texas Instruments as described here:
 * http://www.beagleboard.org
 *
 * This board is based on OMAP3530.
 * More on OMAP3530 (including documentation can be found here):
 * http://focus.ti.com/docs/prod/folders/print/omap3530.html
 *
 * This file provides initialization in two stages:
 * @li boot time initialization - do basics required to get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for Beagle.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from arch/arm/boards/omap/board-sdp343x.c
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <sizes.h>
#include <io.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>
#include <mach/ehci.h>
#include <mach/omap3-devices.h>
#include <i2c/i2c.h>
#include <linux/err.h>
#include <usb/ehci.h>

#ifdef CONFIG_DRIVER_SERIAL_NS16550

/**
 * @brief UART serial port initialization - remember to enable COM clocks in
 * arch
 *
 * @return result of device registration
 */
static int beagle_console_init(void)
{
	barebox_set_model("Texas Instruments beagle");
	barebox_set_hostname("beagle");

	omap3_add_uart3();

	return 0;
}
console_initcall(beagle_console_init);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

#ifdef CONFIG_USB_EHCI_OMAP
static struct omap_hcd omap_ehci_pdata = {
	.port_mode[0] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[1] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[2] = EHCI_HCD_OMAP_MODE_UNKNOWN,
	.phy_reset  = 1,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = 147,
	.reset_gpio_port[2]  = -EINVAL
};

static struct ehci_platform_data ehci_pdata = {
	.flags = 0,
};
#endif /* CONFIG_USB_EHCI_OMAP */

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("twl4030", 0x48),
	},
};

static struct gpmc_nand_platform_data nand_plat = {
	.device_width = 16,
	.ecc_mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	.nand_cfg = &omap3_nand_cfg,
};

static int beagle_mem_init(void)
{
	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(beagle_mem_init);

static int beagle_devices_init(void)
{
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	omap3_add_i2c1(NULL);

#ifdef CONFIG_USB_EHCI_OMAP
	if (ehci_omap_init(&omap_ehci_pdata) >= 0)
		omap3_add_ehci(&ehci_pdata);
#endif /* CONFIG_USB_EHCI_OMAP */
#ifdef CONFIG_OMAP_GPMC
	/* WP is made high and WAIT1 active Low */
	gpmc_generic_init(0x10);
#endif
	omap_add_gpmc_nand_device(&nand_plat);

	omap3_add_mmc1(NULL);

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_OMAP3_BEAGLE);

	return 0;
}
device_initcall(beagle_devices_init);
