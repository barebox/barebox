/* SPDX-License-Identifier: GPL-2.0-only */

#include <mach/imx/devices.h>
#include <mach/imx/imx1-regs.h>

static inline struct device *imx1_add_uart0(void)
{
	return imx_add_uart_imx1((void *)MX1_UART1_BASE_ADDR, 0);
}

static inline struct device *imx1_add_uart1(void)
{
	return imx_add_uart_imx1((void *)MX1_UART2_BASE_ADDR, 1);
}
