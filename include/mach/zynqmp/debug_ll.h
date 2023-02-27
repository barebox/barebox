/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>

#define ZYNQMP_UART0_BASE		0xFF000000
#define ZYNQMP_UART1_BASE		0xFF010000
#define ZYNQMP_UART_BASE		ZYNQMP_UART0_BASE
#define ZYNQMP_DEBUG_LL_UART_BASE	ZYNQMP_UART_BASE

#define ZYNQMP_UART_RXTXFIFO	0x30
#define ZYNQMP_UART_CHANNEL_STS	0x2C

#define ZYNQMP_UART_STS_TFUL	(1 << 4)
#define ZYNQMP_UART_TXDIS		(1 << 5)

static inline void PUTC_LL(int c)
{
	void __iomem *base = (void __iomem *)ZYNQMP_DEBUG_LL_UART_BASE;

	if (readl(base) & ZYNQMP_UART_TXDIS)
		return;

	while ((readl(base + ZYNQMP_UART_CHANNEL_STS) & ZYNQMP_UART_STS_TFUL) != 0)
		;

	writel(c, base + 0x30);
}

#endif
