/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix, Michel Stam,
 * Fugro Intersite
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
 *
 */

/**
 * @file
 * @brief Generic PC support to let barebox acting as a boot loader
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/syslib.h>
#include <ns16550.h>
#include <linux/err.h>

static struct NS16550_plat serial_plat = {
	.clock = 1843200,
};

static int pc_console_init(void)
{
	barebox_set_model("X86 generic barebox");
	barebox_set_hostname("x86");

	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, 0x3f8, 8, IORESOURCE_IO,
			   &serial_plat);
	add_ns16550_device(DEVICE_ID_DYNAMIC, 0x2f8, 8, IORESOURCE_IO,
			   &serial_plat);

	return 0;
}
console_initcall(pc_console_init);
