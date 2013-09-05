/*
 * Copyright (C) 2009 Carlo Caione <carlo@carlocaione.org>
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

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>

#include <mach/core.h>

static int rpi_mem_init(void)
{
	bcm2835_add_device_sdram(0);
	return 0;
}
mem_initcall(rpi_mem_init);

static int rpi_console_init(void)
{
	barebox_set_model("RaspberryPi (BCM2835/ARM1176JZF-S)");
	barebox_set_hostname("rpi");

	bcm2835_register_uart();
	return 0;
}
console_initcall(rpi_console_init);

static int rpi_devices_init(void)
{
	armlinux_set_architecture(MACH_TYPE_BCM2708);
	armlinux_set_bootparams((void *)(0x00000100));
	return 0;
}

device_initcall(rpi_devices_init);
