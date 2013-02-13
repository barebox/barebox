/*
 * Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagniol@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <linux/amba/serial.h>
#include <io.h>

#define UART_BASE 0xfff36000

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
