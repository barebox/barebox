/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_SOCFPGA_DEBUG_LL_H__
#define   __MACH_SOCFPGA_DEBUG_LL_H__

#include <io.h>
#include <mach/socfpga/soc64-regs.h>

#define __SOCFPGA_UART_BASE(num)	SOCFPGA_UART##num##_ADDRESS
#define SOCFPGA_UART_BASE(num)		__SOCFPGA_UART_BASE(num)

#ifdef CONFIG_DEBUG_SOCFPGA_UART

#define UART_BASE			SOCFPGA_UART_BASE(CONFIG_DEBUG_SOCFPGA_UART_PORT)

#if defined(CONFIG_ARCH_SOCFPGA_CYCLONE5)
static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writeb(val, base + (reg << 2));
}
#else
static inline uint8_t debug_ll_read_reg(void __iomem *base, int reg)
{
	return readl(base + (reg << 2));
}

static inline void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val)
{
	writel(val, base + (reg << 2));
}
#endif

#include <debug_ll/ns16550.h>

static inline void socfpga_uart_setup(void *base)
{
	unsigned int div;

	div = debug_ll_ns16550_calc_divisor(CONFIG_DEBUG_SOCFPGA_UART_CLOCK);
	debug_ll_ns16550_init(base, div);
}

static inline void socfpga_uart_setup_ll(void)
{
	void __iomem *base = IOMEM(UART_BASE);

	socfpga_uart_setup(base);
}

static inline void socfpga_uart_putc(void *base, int c)
{
	debug_ll_ns16550_putc(base, c);
}

static inline void PUTC_LL(char c)
{
	void __iomem *base = IOMEM(UART_BASE);

	socfpga_uart_putc(base, c);
}
#else
static inline void socfpga_uart_setup_ll(void) {}
static inline void socfpga_uart_putc(void *base, int c) {}
static inline void socfpga_uart_setup(void *base) {}
#endif
#endif /* __MACH_SOCFPGA_DEBUG_LL_H__ */
