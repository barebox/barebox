/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <ns16550.h>
#include <mach/hardware.h>
#include <io.h>
#include <asm/common.h>

static struct NS16550_plat serial_plat = {
	.clock = 25804800,
	.shift = 0,
};

static int dir320_console_init(void)
{
	barebox_set_model("D-Link DIR-320");
	barebox_set_hostname("dir320");

	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, DEBUG_LL_UART_ADDR, 8,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(dir320_console_init);
