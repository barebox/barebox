
#include <mach/devices.h>

static inline struct device_d *imx21_add_uart0(void)
{
	return imx_add_uart((void *)IMX_UART1_BASE, 0);
}

static inline struct device_d *imx21_add_uart1(void)
{
	return imx_add_uart((void *)IMX_UART2_BASE, 1);
}

static inline struct device_d *imx21_add_uart2(void)
{
	return imx_add_uart((void *)IMX_UART3_BASE, 2);
}

static inline struct device_d *imx21_add_uart3(void)
{
	return imx_add_uart((void *)IMX_UART4_BASE, 3);
}

static inline struct device_d *imx21_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)0xDF003000, pdata);
}

static inline struct device_d *imx21_add_fb(struct imx_fb_platform_data *pdata)
{
	return imx_add_fb((void *)0x10021000, pdata);
}

