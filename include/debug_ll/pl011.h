#ifndef __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__
#define __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__

#ifndef DEBUG_LL_UART_ADDR
#error DEBUG_LL_UART_ADDR is undefined!
#endif

#include <io.h>
#include <linux/amba/serial.h>

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while (readl(DEBUG_LL_UART_ADDR + UART01x_FR) & UART01x_FR_TXFF)
		;

	/* Send the character */
	writel(c, DEBUG_LL_UART_ADDR + UART01x_DR);

	/* Wait to make sure it hits the line, in case we die too soon. */
	while (readl(DEBUG_LL_UART_ADDR + UART01x_FR) & UART01x_FR_TXFF)
		;
}

#endif /* __INCLUDE_ARM_ASM_DEBUG_LL_PL011_H__ */
