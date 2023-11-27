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

#ifndef __MACH_OMAP_DEBUG_LL_H__
#define   __MACH_OMAP_DEBUG_LL_H__

#include <io.h>
#include <mach/omap/omap3-silicon.h>
#include <mach/omap/omap4-silicon.h>
#include <mach/omap/am33xx-silicon.h>

#ifdef CONFIG_DEBUG_OMAP_UART

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

static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writeb(val, base + (reg << 2));
}

#include <debug_ll/ns16550.h>

static inline void omap_debug_ll_init(void)
{
	void __iomem *base = (void *)OMAP_UART_BASE(OMAP_DEBUG_SOC,
			CONFIG_DEBUG_OMAP_UART_PORT);
	unsigned int divisor;

	divisor = debug_ll_ns16550_calc_divisor(48000000);
	debug_ll_ns16550_init(base, divisor);
}

static inline void PUTC_LL(int c)
{
	void __iomem *base = (void *)OMAP_UART_BASE(OMAP_DEBUG_SOC,
			CONFIG_DEBUG_OMAP_UART_PORT);

	debug_ll_ns16550_putc(base, c);
}

#else
static inline void omap_debug_ll_init(void)
{
}
#endif

#endif /* __MACH_OMAP_DEBUG_LL_H__ */
