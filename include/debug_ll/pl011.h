/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__
#define __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__

#include <io.h>
#include <linux/amba/serial.h>

static inline void debug_ll_pl011_putc(void __iomem *base, int c)
{
	/* Wait until there is space in the FIFO */
	while (readl(base + UART01x_FR) & UART01x_FR_TXFF)
		;

	/* Send the character */
	writel(c, base + UART01x_DR);
}

#ifdef CONFIG_DEBUG_LL

#ifndef DEBUG_LL_UART_ADDR
#error DEBUG_LL_UART_ADDR is undefined!
#endif

static inline void PUTC_LL(char c)
{
	void __iomem *base = IOMEM(DEBUG_LL_UART_ADDR);

	debug_ll_pl011_putc(base, c);

	/* Wait to make sure it hits the line, in case we die too soon. */
	while (readl(base + UART01x_FR) & UART01x_FR_TXFF)
		;
}

#endif

#endif /* __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__ */
