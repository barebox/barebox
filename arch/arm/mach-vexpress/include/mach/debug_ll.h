/*
 * Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagniol@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <linux/amba/serial.h>
#include <io.h>

#define DEBUG_LL_PHYS_BASE		0x10000000
#define DEBUG_LL_PHYS_BASE_RS1		0x1c000000

#ifdef MP
#define UART_BASE DEBUG_LL_PHYS_BASE
#else
#define UART_BASE DEBUG_LL_PHYS_BASE_RS1
#endif

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while (readl(UART_BASE + UART01x_FR) & UART01x_FR_TXFF);

	/* Send the character */
	writel(c, UART_BASE + UART01x_DR);

	/* Wait to make sure it hits the line, in case we die too soon. */
	while (readl(UART_BASE + UART01x_FR) & UART01x_FR_TXFF);
}
#endif
