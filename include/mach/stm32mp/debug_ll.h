/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_STM32MP1_DEBUG_LL_H
#define __MACH_STM32MP1_DEBUG_LL_H

#include <io.h>
#include <mach/stm32mp/stm32.h>
#include <linux/bitops.h>

#define DEBUG_LL_UART_ADDR	STM32_UART4_BASE

#define CR1_OFFSET	0x00
#define CR3_OFFSET	0x08
#define BRR_OFFSET	0x0c
#define ISR_OFFSET	0x1c
#define ICR_OFFSET	0x20
#define RDR_OFFSET	0x24
#define TDR_OFFSET	0x28

#define USART_ISR_TXE	BIT(7)

static inline void stm32_serial_putc(void *ctx, int c)
{
	void __iomem *base = IOMEM(ctx);

	writel(c, base + TDR_OFFSET);

	while ((readl(base + ISR_OFFSET) & USART_ISR_TXE) == 0);
}

static inline void PUTC_LL(int c)
{
	stm32_serial_putc(IOMEM(DEBUG_LL_UART_ADDR), c);
}

#endif /* __MACH_STM32MP1_DEBUG_LL_H */
