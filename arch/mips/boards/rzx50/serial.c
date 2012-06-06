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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <ns16550.h>
#include <mach/jz4750d_regs.h>
#include <io.h>
#include <asm/common.h>

#define JZ4750D_UART_SHIFT	2

#define ier		(1 << JZ4750D_UART_SHIFT)
#define fcr		(2 << JZ4750D_UART_SHIFT)

static void jz4750d_serial_reg_write(unsigned int val, unsigned long base,
	unsigned char reg_offset)
{
	switch (reg_offset) {
	case fcr:
		val |= 0x10; /* Enable uart module */
		break;
	case ier:
		val |= (val & 0x4) << 2;
		break;
	default:
		break;
	}

	writeb(val & 0xff, (void *)(base + reg_offset));
}

static struct NS16550_plat serial_plat = {
	.clock = 12000000,
	.shift = JZ4750D_UART_SHIFT,
	.reg_write = &jz4750d_serial_reg_write,
};

static int rzx50_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(-1, UART1_BASE, 8 << JZ4750D_UART_SHIFT,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(rzx50_console_init);
