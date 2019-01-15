#include <common.h>
#include <io.h>
#include <mach/am3xxx-silicon.h>

/* UART Defines */
#define UART_SYSCFG_OFFSET	0x54
#define UART_SYSSTS_OFFSET	0x58

#define UART_CLK_RUNNING_MASK	0x1
#define UART_RESET		(0x1 << 1)
#define UART_SMART_IDLE_EN	(0x1 << 0x3)

void am3xxx_uart_soft_reset(void __iomem *uart_base)
{
	int reg;

	reg = readl(uart_base + UART_SYSCFG_OFFSET);
	reg |= UART_RESET;
	writel(reg, (uart_base + UART_SYSCFG_OFFSET));

	while ((readl(uart_base + UART_SYSSTS_OFFSET) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK)
		;

	/* Disable smart idle */
	reg = readl((uart_base + UART_SYSCFG_OFFSET));
	reg |= UART_SMART_IDLE_EN;
	writel(reg, (uart_base + UART_SYSCFG_OFFSET));
}

void am33xx_uart_soft_reset(void __iomem *uart_base)
	__alias(am3xxx_uart_soft_reset);