/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/**
 * @file
 * @brief x86 Architecture Initialization routines
 */

#include <asm/io.h>

/** to work with the 8250 UART driver implementation we need this function */
unsigned int x86_uart_read(unsigned long base, unsigned char reg_idx)
{
	return inb(base + reg_idx);
}

/** to work with the 8250 UART driver implementation we need this function */
void x86_uart_write(unsigned int val, unsigned long base, unsigned char reg_idx)
{
	outb(val, base + reg_idx);
}
