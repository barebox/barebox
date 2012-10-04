
#include <mach/imx-regs.h>
#include <mach/devices.h>

static inline struct device_d *imx31_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX31_CSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx31_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX31_CSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx31_add_spi2(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX31_CSPI3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx31_add_uart0(void)
{
	return imx_add_uart((void *)MX31_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx31_add_uart1(void)
{
	return imx_add_uart((void *)MX31_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx31_add_uart2(void)
{
	return imx_add_uart((void *)MX31_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx31_add_uart3(void)
{
	return imx_add_uart((void *)MX31_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx31_add_uart4(void)
{
	return imx_add_uart((void *)MX31_UART5_BASE_ADDR, 4);
}

static inline struct device_d *imx31_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)MX31_NFC_BASE_ADDR, pdata);
}

static inline struct device_d *imx31_add_fb(struct imx_ipu_fb_platform_data *pdata)
{
	return imx_add_ipufb((void *)MX31_IPU_CTRL_BASE_ADDR, pdata);
}
