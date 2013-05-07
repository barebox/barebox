/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on the linux kernel JZ4740 serial support:
 * Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
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
#include <ns16550.h>
#include <io.h>
#include <mach/devices.h>

#define JZ_UART_SHIFT	2

#define ier		(1 << JZ_UART_SHIFT)
#define fcr		(2 << JZ_UART_SHIFT)

static void jz_serial_reg_write(unsigned int val, unsigned long base,
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

struct device_d *jz_add_uart(int id, unsigned long base, unsigned int clock)
{
	struct NS16550_plat *serial_plat;

	serial_plat = xzalloc(sizeof(*serial_plat));

	serial_plat->shift = JZ_UART_SHIFT;
	serial_plat->reg_write = &jz_serial_reg_write;
	serial_plat->clock = clock;

	return add_ns16550_device(id, base, 8 << JZ_UART_SHIFT,
			IORESOURCE_MEM_8BIT, serial_plat);
}
