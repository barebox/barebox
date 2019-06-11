#ifndef __MACH_STM32MP1_DEBUG_LL_H
#define __MACH_STM32MP1_DEBUG_LL_H

#include <io.h>
#include <mach/stm32.h>

#define DEBUG_LL_UART_ADDR	STM32_UART4_BASE

#define CR1_OFFSET	0x00
#define CR3_OFFSET	0x08
#define BRR_OFFSET	0x0c
#define ISR_OFFSET	0x1c
#define ICR_OFFSET	0x20
#define RDR_OFFSET	0x24
#define TDR_OFFSET	0x28

#define USART_ISR_TXE	BIT(7)

static inline void PUTC_LL(int c)
{
	void __iomem *base = IOMEM(DEBUG_LL_UART_ADDR);

	writel(c, base + TDR_OFFSET);

	while ((readl(base + ISR_OFFSET) & USART_ISR_TXE) == 0);
}

#endif /* __MACH_STM32MP1_DEBUG_LL_H */
