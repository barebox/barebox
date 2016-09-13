#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <common.h>
#include <io.h>
#include <mach/rk3188-regs.h>
#include <mach/rk3288-regs.h>

#ifdef CONFIG_ARCH_RK3188

#define UART_CLOCK		100000000
#define RK_DEBUG_SOC		RK3188
#define serial_out(a, v)	writeb(v, a)
#define serial_in(a)		readb(a)

#elif defined CONFIG_ARCH_RK3288

#define UART_CLOCK		24000000
#define RK_DEBUG_SOC		RK3288
#define serial_out(a, v)	writel(v, a)
#define serial_in(a)		readl(a)

#endif

#define __RK_UART_BASE(soc, num) soc##_UART##num##_BASE
#define RK_UART_BASE(soc, num) __RK_UART_BASE(soc, num)

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

static inline void INIT_LL(void)
{
	void __iomem *base = IOMEM(RK_UART_BASE(RK_DEBUG_SOC,
		CONFIG_DEBUG_ROCKCHIP_UART_PORT));
	unsigned int divisor = DIV_ROUND_CLOSEST(UART_CLOCK,
		16 * CONFIG_BAUDRATE);

	serial_out(base + LCR, 0x00);
	serial_out(base + IER, 0x00);
	serial_out(base + MDR, 0x07);
	serial_out(base + LCR, LCR_BKSE);
	serial_out(base + DLL, divisor & 0xff);
	serial_out(base + DLM, divisor >> 8);
	serial_out(base + LCR, 0x03);
	serial_out(base + MCR, 0x03);
	serial_out(base + FCR, 0x07);
	serial_out(base + MDR, 0x00);
}

static inline void PUTC_LL(char c)
{
	void __iomem *base = IOMEM(RK_UART_BASE(RK_DEBUG_SOC,
		CONFIG_DEBUG_ROCKCHIP_UART_PORT));

	/* Wait until there is space in the FIFO */
	while ((serial_in(base + LSR) & LSR_THRE) == 0)
		;
	/* Send the character */
	serial_out(base + THR, c);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((serial_in(base + LSR) & LSR_THRE) == 0)
		;
}
#endif
