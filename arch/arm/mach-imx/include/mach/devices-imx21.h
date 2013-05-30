
#include <mach/devices.h>
#include <mach/imx21-regs.h>

static inline struct device_d *imx21_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX21_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx21_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX21_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx21_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX21_UART2_BASE_ADDR, 2);
}

static inline struct device_d *imx21_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX21_UART2_BASE_ADDR, 3);
}

static inline struct device_d *imx21_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)0xDF003000, pdata);
}

static inline struct device_d *imx21_add_fb(struct imx_fb_platform_data *pdata)
{
	return imx_add_fb((void *)0x10021000, pdata);
}

