/*
 * Copyright (C) 2013
 *  Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#define UART_BASE	0xf1012000
#define UARTn_BASE(n)	(UART_BASE + ((n) * 0x100))
#define UART_THR	0x00
#define UART_LSR	0x14
#define   LSR_THRE	BIT(5)

#define EARLY_UART	UARTn_BASE(CONFIG_MVEBU_CONSOLE_UART)

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while (!(readl(EARLY_UART + UART_LSR) & LSR_THRE))
		;

	/* Send the character */
	writel(c, EARLY_UART + UART_THR);

	/* Wait to make sure it hits the line */
	while (!(readl(EARLY_UART + UART_LSR) & LSR_THRE))
		;
}
#endif
