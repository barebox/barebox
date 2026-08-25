/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_PXA_DEBUG_LL_H__
#define __MACH_PXA_DEBUG_LL_H__

#include <io.h>
#include <linux/bits.h>

/*
 * The PXA UARTs are NS16550 compatible, but the registers are 32bit
 * wide and thus spaced four bytes apart. In addition to that the unit
 * has to be enabled explicitly with the UUE bit in the IER register.
 */
#define PXA_UART_IER_UUE	BIT(6)

#define PXA_FFUART_BASE		0x40100000
#define PXA_BTUART_BASE		0x40200000
#define PXA_STUART_BASE		0x40700000

#ifdef CONFIG_DEBUG_PXA_UART

#if CONFIG_DEBUG_PXA_UART_PORT == 0
#define PXA_DEBUG_UART_BASE	PXA_FFUART_BASE
#elif CONFIG_DEBUG_PXA_UART_PORT == 1
#define PXA_DEBUG_UART_BASE	PXA_BTUART_BASE
#elif CONFIG_DEBUG_PXA_UART_PORT == 2
#define PXA_DEBUG_UART_BASE	PXA_STUART_BASE
#else
#error "Invalid CONFIG_DEBUG_PXA_UART_PORT"
#endif

static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readl(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writel(val, base + (reg << 2));
}

#include <debug_ll/ns16550.h>

static inline void pxa_debug_ll_init(void)
{
	void __iomem *base = IOMEM(PXA_DEBUG_UART_BASE);
	uint16_t divisor;

	/* The UART clock is a fixed 14.857MHz on all PXA variants */
	divisor = debug_ll_ns16550_calc_divisor(14857000);
	debug_ll_ns16550_init(base, divisor);

	/*
	 * debug_ll_ns16550_init() has cleared IER to disable the
	 * interrupts, which on PXA also disabled the unit. Enable it
	 * again, interrupts stay disabled.
	 */
	debug_ll_write_reg(base, NS16550_IER, PXA_UART_IER_UUE);
}

static inline void PUTC_LL(char c)
{
	debug_ll_ns16550_putc(IOMEM(PXA_DEBUG_UART_BASE), c);
}

#else

static inline void pxa_debug_ll_init(void)
{
}

#endif /* CONFIG_DEBUG_PXA_UART */

#endif /* __MACH_PXA_DEBUG_LL_H__ */
