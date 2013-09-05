/*
 * Copyright (C) 2011
 * Author: Jan Weitzel <j.weitzel@phytec.de>
 * based on arch/arm/mach-versatile/include/mach/debug_ll.h
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <io.h>

#ifdef CONFIG_ARCH_OMAP3
#include <mach/omap3-silicon.h>

#ifdef CONFIG_OMAP_UART1
#define UART_BASE	OMAP3_UART1_BASE
#else
#define UART_BASE	OMAP3_UART3_BASE
#endif

#endif

#ifdef CONFIG_ARCH_OMAP4
#include <mach/omap4-silicon.h>
#ifdef CONFIG_OMAP_UART1
#define UART_BASE	OMAP44XX_UART1_BASE
#else
#define UART_BASE	OMAP44XX_UART3_BASE
#endif
#endif

#ifdef CONFIG_ARCH_AM33XX
#include <mach/am33xx-silicon.h>
#define UART_BASE	AM33XX_UART0_BASE
#endif

#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LCR_BKSE	0x80	/* Bank select enable */
#define LSR		(5 << 2)
#define THR		(0 << 2)
#define DLL		(0 << 2)
#define IER		(1 << 2)
#define DLM		(1 << 2)
#define FCR		(2 << 2)
#define LCR		(3 << 2)
#define MCR		(4 << 2)
#define MDR		(8 << 2)

static inline void omap_uart_lowlevel_init(void)
{
	writeb(0x00, UART_BASE + LCR);
	writeb(0x00, UART_BASE + IER);
	writeb(0x07, UART_BASE + MDR);
	writeb(LCR_BKSE, UART_BASE + LCR);
	writeb(26, UART_BASE + DLL); /* 115200 */
	writeb(0, UART_BASE + DLM);
	writeb(0x03, UART_BASE + LCR);
	writeb(0x03, UART_BASE + MCR);
	writeb(0x07, UART_BASE + FCR);
	writeb(0x00, UART_BASE + MDR);
}
static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, UART_BASE + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
}
#endif
