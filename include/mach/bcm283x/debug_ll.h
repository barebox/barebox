/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_BCM2835_DEBUG_LL_H__
#define __MACH_BCM2835_DEBUG_LL_H__

#include <mach/bcm283x/platform.h>
#include <io.h>

#ifdef CONFIG_DEBUG_RPI1_UART

static inline void debug_ll_init(void)
{
	/* Configured by ROM */
}

#define DEBUG_LL_UART_ADDR BCM2835_PL011_BASE
#include <debug_ll/pl011.h>

#elif defined CONFIG_DEBUG_RPI2_3_UART

static inline void debug_ll_init(void)
{
	/* Configured by ROM */
}

#define DEBUG_LL_UART_ADDR BCM2836_PL011_BASE
#include <debug_ll/pl011.h>

#elif defined CONFIG_DEBUG_RPI3_MINI_UART

static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writeb(val, base + (reg << 2));
}

#define BCM2836_AUX_CLOCK_ENB		0x3f215004 /* BCM2835 AUX Clock enable register */
#define BCM2836_AUX_CLOCK_EN_UART	BIT(0) /* Bit 0 enables the Miniuart */

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
	uint16_t divisor;
	void __iomem *base = IOMEM(BCM2836_MINIUART_BASE);

	writeb(BCM2836_AUX_CLOCK_EN_UART, BCM2836_AUX_CLOCK_ENB);

	divisor = debug_ll_ns16550_calc_divisor(250000000 * 2);
	debug_ll_ns16550_init(base, divisor);
}

static inline void PUTC_LL(int c)
{
	void __iomem  *base = IOMEM(BCM2836_MINIUART_BASE);

	debug_ll_ns16550_putc(base, c);
}

#elif defined CONFIG_DEBUG_RPI4_MINI_UART

static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writeb(val, base + (reg << 2));
}

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
	/* Configured by ROM */
}

static inline void PUTC_LL(int c)
{
	void __iomem *base = IOMEM(BCM2711_MINIUART_BASE);

	debug_ll_ns16550_putc(base, c);
}

#else

static inline void debug_ll_init(void)
{
}

#endif

#endif /* __MACH_BCM2835_DEBUG_LL_H__ */
