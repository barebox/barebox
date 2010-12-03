
#include <mach/imx-regs.h>
#include <mach/devices.h>

#if 0
static inline struct device_d *imx31_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)IMX_SPI1_BASE, 0, pdata);
}

static inline struct device_d *imx31_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)IMX_SPI2_BASE, 1, pdata);
}
#endif

static inline struct device_d *imx31_add_uart0(void)
{
	return imx_add_uart((void *)IMX_UART1_BASE, 0);
}

static inline struct device_d *imx31_add_uart1(void)
{
	return imx_add_uart((void *)IMX_UART2_BASE, 1);
}

static inline struct device_d *imx31_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)0xb8000000, pdata);
}

static inline struct device_d *imx31_add_fb(struct imx_ipu_fb_platform_data *pdata)
{
	return imx_add_ipufb((void *)IPU_BASE, pdata);
}
