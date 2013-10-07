/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <init.h>
#include <mach/devices.h>
#include <mach/jz4750d_regs.h>

static int rzx50_console_init(void)
{
	barebox_set_hostname("rzx50");

	/* Register the serial port */
	jz_add_uart(DEVICE_ID_DYNAMIC, UART1_BASE, 12000000);

	return 0;
}
console_initcall(rzx50_console_init);
