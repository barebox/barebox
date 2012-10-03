/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <stdio.h>
#include <common.h>
#include <asm/nios2-io.h>
#include <io.h>

void early_putc(char ch)
{
	struct nios_uart *uart = (struct nios_uart *)NIOS_SOPC_UART_BASE;

	while ((readl(&uart->status) & NIOS_UART_TRDY) == 0);
	writel((unsigned char)ch, &uart->txdata);
}

void early_puts(const char *s)
{
	while (*s != 0)
		early_putc(*s++);
}

int early_printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[50];

	va_start(args, fmt);

	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	early_puts(printbuffer);

	return 0;
}
