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
#include <mach/omap3-silicon.h>
#include <mach/omap4-silicon.h>
#include <mach/am33xx-silicon.h>

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

static inline void omap_uart_lowlevel_init(void __iomem *base)
{
	writeb(0x00, base + LCR);
	writeb(0x00, base + IER);
	writeb(0x07, base + MDR);
	writeb(LCR_BKSE, base + LCR);
	writeb(26, base + DLL); /* 115200 */
	writeb(0, base + DLM);
	writeb(0x03, base + LCR);
	writeb(0x03, base + MCR);
	writeb(0x07, base + FCR);
	writeb(0x00, base + MDR);
}

#ifdef CONFIG_DEBUG_LL

#ifdef CONFIG_DEBUG_OMAP3_UART
#define OMAP_DEBUG_SOC OMAP3
#elif defined CONFIG_DEBUG_OMAP4_UART
#define OMAP_DEBUG_SOC OMAP44XX
#elif defined CONFIG_DEBUG_AM33XX_UART
#define OMAP_DEBUG_SOC AM33XX
#else
#error "unknown OMAP debug uart soc type"
#endif

#define __OMAP_UART_BASE(soc, num) soc##_UART##num##_BASE
#define OMAP_UART_BASE(soc, num) __OMAP_UART_BASE(soc, num)

static inline void PUTC_LL(char c)
{
	void __iomem *base = (void *)OMAP_UART_BASE(OMAP_DEBUG_SOC,
			CONFIG_DEBUG_OMAP_UART_PORT);

	/* Wait until there is space in the FIFO */
	while ((readb(base + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, base + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(base + LSR) & LSR_THRE) == 0);
}
#endif

#endif
