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
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <asm/io.h>

#ifdef CONFIG_ARCH_OMAP3
#include <mach/omap3-silicon.h>

#ifdef CONFIG_OMAP3EVM_UART1
#define UART_BASE	OMAP_UART1_BASE
#else
#define UART_BASE	OMAP_UART3_BASE
#endif

#endif

#ifdef CONFIG_ARCH_OMAP4
#include <mach/omap4-silicon.h>
#define UART_BASE	OMAP44XX_UART3_BASE
#endif

#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LSR		(5 << 2)
#define THR		(0 << 2)

static inline void putc(char c)
{
	/* Wait until there is space in the FIFO */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, UART_BASE + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
}
#endif
