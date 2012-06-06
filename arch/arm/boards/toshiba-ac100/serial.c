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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <ns16550.h>
#include <asm/io.h>
#include <asm/common.h>
#include <mach/iomap.h>

static struct NS16550_plat serial_plat = {
	.clock = 0x75 * 115200 * 16 /* MODE_X_DIV */,
	.shift = 2,
};

static int ac100_serial_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(-1, TEGRA_UARTA_BASE, 8 << serial_plat.shift,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(ac100_serial_console_init);
