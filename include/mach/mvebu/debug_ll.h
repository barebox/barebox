/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com> */

#ifndef __MACH_MVEBU_DEBUG_LL_H__
#define __MACH_MVEBU_DEBUG_LL_H__

#include <io.h>
#include <linux/bits.h>

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
#endif /* __MACH_MVEBU_DEBUG_LL_H__ */
