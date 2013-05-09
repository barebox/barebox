/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>

#define UART_BASE 0xd0012000
#define UART_THR  0x0
#define UART_LSR  0x14
#define   UART_LSR_THRE   (1 << 5)

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while (!(readl(UART_BASE + UART_LSR) & UART_LSR_THRE))
		;

	/* Send the character */
	writel(c, UART_BASE + UART_THR)
		;

	/* Wait to make sure it hits the line, in case we die too soon. */
	while (!(readl(UART_BASE + UART_LSR) & UART_LSR_THRE))
		;
}
#endif
