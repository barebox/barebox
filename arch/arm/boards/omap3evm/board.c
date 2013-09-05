/**
 * @file
 * @brief Board Initialization routines for OMAP3EVM.
 *
 * This board is based on OMAP3530.
 * More on OMAP3530 (including documentation can be found here):
 * http://focus.ti.com/docs/prod/folders/print/omap3530.html
 *
 * This file provides initialization in two stages:
 * @li Boot time initialization - just get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li Run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for the EVM.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from arch/arm/boards/omap/board-beagle.c
 */

/*
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * Sanjeev Premi <premi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */


#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <sizes.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-mux.h>
#include <mach/gpmc.h>
#include <errno.h>
#include <generated/mach-types.h>
#include <mach/omap3-devices.h>

/**
 * @brief Initialize the serial port to be used as console.
 *
 * @return result of device registration
 */
static int omap3evm_init_console(void)
{
	barebox_set_model("Texas Instruments omap3evm");
	barebox_set_hostname("omap3evm");

	if (IS_ENABLED(CONFIG_OMAP_UART1))
		omap3_add_uart1();
	if (IS_ENABLED(CONFIG_OMAP_UART3))
		omap3_add_uart3();

	return 0;
}
console_initcall(omap3evm_init_console);

static int omap3evm_mem_init(void)
{
	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(omap3evm_mem_init);

static int omap3evm_init_devices(void)
{
#ifdef CONFIG_OMAP_GPMC
	/*
	 * WP is made high and WAIT1 active Low
	 */
	gpmc_generic_init(0x10);
#endif
	omap3_add_mmc1(NULL);

        armlinux_set_bootparams((void *)0x80000100);
        armlinux_set_architecture(MACH_TYPE_OMAP3EVM);

	return 0;
}
device_initcall(omap3evm_init_devices);
