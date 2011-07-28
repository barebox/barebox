#include <mach/devices.h>

static inline struct device_d *imx1_add_uart0(void)
{
	return imx_add_uart((void *)IMX_UART1_BASE, 0);
}

static inline struct device_d *imx1_add_uart1(void)
{
	return imx_add_uart((void *)IMX_UART2_BASE, 1);
}
