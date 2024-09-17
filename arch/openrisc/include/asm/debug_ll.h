/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_DEBUG_LL__
#define __ASM_DEBUG_LL__

#include <io.h>

#if defined CONFIG_DEBUG_OPENRISC_NS16550

#define DEBUG_LL_UART_BASE	IOMEM(0x90000000)

static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readb(base + reg);
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writeb(val, base + reg);
}

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
	/* already configured */
}

static inline void PUTC_LL(int c)
{
	debug_ll_ns16550_putc(DEBUG_LL_UART_BASE, c);
}

#endif

#endif
