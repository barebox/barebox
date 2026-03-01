/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_DEBUG_LL_H__
#define __ASM_DEBUG_LL_H__

#include <io.h>

#ifdef CONFIG_DEBUG_QEMU_PPCE500

#include <asm/board-qemu-e500.h>

/*
 * QEMU ppce500 NS16550 UART at CCSRBAR + 0x4500
 * Clock is platform clock (400 MHz default)
 */
#define DEBUG_LL_UART_BASE	IOMEM(CFG_CCSRBAR + 0x4500)
#define DEBUG_LL_UART_CLK	CFG_SYS_CLK_FREQ

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
	uint16_t divisor;

	divisor = debug_ll_ns16550_calc_divisor(DEBUG_LL_UART_CLK);
	debug_ll_ns16550_init(DEBUG_LL_UART_BASE, divisor);
}

static inline void PUTC_LL(int c)
{
	debug_ll_ns16550_putc(DEBUG_LL_UART_BASE, c);
}

#else

static inline void debug_ll_init(void)
{
}

#endif /* CONFIG_DEBUG_QEMU_PPCE500 */

#endif /* __ASM_DEBUG_LL_H__ */
