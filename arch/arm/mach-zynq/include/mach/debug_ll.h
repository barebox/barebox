/*
 * based on mach-imx/include/mach/debug_ll.h
 */

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>
#include <mach/zynq7000-regs.h>

#ifndef CONFIG_ZYNQ_DEBUG_LL_UART_BASE
#warning define ZYNQ_DEBUG_LL_UART_BASE properly for debug_ll
#define ZYNQ_DEBUG_LL_UART_BASE ZYNQ_UART1_BASE_ADDR
#else
#define ZYNQ_DEBUG_LL_UART_BASE CONFIG_ZYNQ_DEBUG_LL_UART_BASE
#endif

#define ZYNQ_UART_RXTXFIFO	0x30
#define ZYNQ_UART_CHANNEL_STS	0x2C

#define ZYNQ_UART_STS_TFUL	(1 << 4)
#define ZYNQ_UART_TXDIS		(1 << 5)

static inline void PUTC_LL(int c)
{
	void __iomem *base = (void __iomem *)ZYNQ_DEBUG_LL_UART_BASE;

	if (readl(base) & ZYNQ_UART_TXDIS)
		return;

	while ((readl(base + ZYNQ_UART_CHANNEL_STS) & ZYNQ_UART_STS_TFUL) != 0)
		;

	writel(c, base + ZYNQ_UART_RXTXFIFO);
}

#endif
