/*
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
 * @brief SDP3430 Specific Board Initialization routines
 */

/**
 * @page ti_SDP3430 Texas Instruments SDP3430
 *
 * SDP3430 from Texas Instruments as described here:
 * http://www.ti.com/omap3430_devplatform
 * This file provides initialization in two stages:
 * @li boot time initialization - do basics required to get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for SDP3430.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-devices.h>
#include <mach/gpmc.h>
#include <errno.h>

/**
 * @brief UART serial port initialization - remember to enable COM clocks in arch
 *
 * @return result of device registration
 */
static int sdp3430_console_init(void)
{
	barebox_set_model("Texas Instruments SDP343x");
	barebox_set_hostname("sdp343x");

	omap3_add_uart3();

	return 0;
}

console_initcall(sdp3430_console_init);

static int sdp3430_mem_init(void)
{
	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(sdp3430_mem_init);

static int sdp3430_devices_init(void)
{
#ifdef CONFIG_OMAP_GPMC
	/* WP is made high and WAIT1 active Low */
	gpmc_generic_init(0x10);
#endif

	return 0;
}

device_initcall(sdp3430_devices_init);
