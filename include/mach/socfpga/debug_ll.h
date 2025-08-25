/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_SOCFPGA_DEBUG_LL_H__
#define   __MACH_SOCFPGA_DEBUG_LL_H__

#include <io.h>
#include <errno.h>

#ifdef CONFIG_DEBUG_LL
#define UART_BASE	CONFIG_DEBUG_SOCFPGA_UART_PHYS_ADDR
#endif

#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LSR_TEMT	0x40

#define LCR_BKSE	0x80	/* Bank select enable */
#define LCRVAL		0x3
#define MCRVAL		0x3
#define FCRVAL		0xc1

#define RBR		0x0
#define DLL		0x0
#define IER		0x4
#define DLM		0x4
#define FCR		0x8
#define LCR		0xc
#define MCR		0x10
#define LSR		0x14
#define MSR		0x18
#define SCR		0x1c
#define THR		0x30

static inline void socfpga_gen5_uart_putc(void *base, int c)
{
	/* Wait until there is space in the FIFO */
	while ((readb(base + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, base + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(base + LSR) & LSR_THRE) == 0);
}

static inline void socfpga_uart_putc(void *base, int c)
{
	/* Wait until there is space in the FIFO */
	while ((readl(base + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writel(c, base + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readl(base + LSR) & LSR_THRE) == 0);
}

#ifdef CONFIG_DEBUG_LL
static inline unsigned int ns16550_calc_divisor(unsigned int clk,
					 unsigned int baudrate)
{
	return (clk / 16 / baudrate);
}

static inline void socfpga_uart_setup_ll(void)
{
	unsigned int div = ns16550_calc_divisor(CONFIG_DEBUG_SOCFPGA_UART_CLOCK,
						115200);

	writel(0x00, UART_BASE + IER);

	writel(LCR_BKSE, UART_BASE + LCR);
	writel(div & 0xff, UART_BASE + DLL);
	writel((div >> 8) & 0xff, UART_BASE + DLM);
	writel(LCRVAL, UART_BASE + LCR);

	writel(MCRVAL, UART_BASE + MCR);
	writel(FCRVAL, UART_BASE + FCR);
}

#if defined(CONFIG_ARCH_SOCFPGA_CYCLONE5)
static inline void PUTC_LL(char c)
{
	void __iomem *base = IOMEM(UART_BASE);

	socfpga_gen5_uart_putc(base, c);
}
#else
static inline void PUTC_LL(char c)
{
	void __iomem *base = IOMEM(UART_BASE);

	socfpga_uart_putc(base, c);
}
#endif

#else
static inline unsigned int ns16550_calc_divisor(unsigned int clk,
					 unsigned int baudrate) {
	return -ENOSYS;
}
static inline void socfpga_uart_setup_ll(void) {}
static inline void PUTC_LL(char c) {}
#endif
#endif /* __MACH_SOCFPGA_DEBUG_LL_H__ */
