#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

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

#ifdef CONFIG_DEBUG_LL
static inline unsigned int ns16550_calc_divisor(unsigned int clk,
					 unsigned int baudrate)
{
	return (clk / 16 / baudrate);
}

static inline void INIT_LL(void)
{
	unsigned int div = ns16550_calc_divisor(CONFIG_DEBUG_SOCFPGA_UART_CLOCK,
						115200);

	while ((readl(UART_BASE + LSR) & LSR_TEMT) == 0);

	writel(0x00, UART_BASE + IER);

	writel(LCR_BKSE, UART_BASE + LCR);
	writel(div & 0xff, UART_BASE + DLL);
	writel((div >> 8) & 0xff, UART_BASE + DLM);
	writel(LCRVAL, UART_BASE + LCR);

	writel(MCRVAL, UART_BASE + MCR);
	writel(FCRVAL, UART_BASE + FCR);
}

#ifdef CONFIG_ARCH_SOCFPGA_ARRIA10
static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while ((readl(UART_BASE + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writel(c, UART_BASE + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readl(UART_BASE + LSR) & LSR_THRE) == 0);
}
#else
static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, UART_BASE + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
}
#endif

#else
static inline unsigned int ns16550_calc_divisor(unsigned int clk,
					 unsigned int baudrate) {
	return -ENOSYS;
}
static inline void INIT_LL(void) {}
static inline void PUTC_LL(char c) {}
#endif
#endif
