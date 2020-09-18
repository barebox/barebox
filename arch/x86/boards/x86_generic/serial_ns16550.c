// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix
// SPDX-FileCopyrightText: 2009 Michel Stam, Fugro Intersite

/**
 * @file
 * @brief Generic PC support to let barebox acting as a boot loader
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/syslib.h>
#include <platform_data/serial-ns16550.h>
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
