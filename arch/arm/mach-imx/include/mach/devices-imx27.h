
#include <mach/devices.h>

static inline struct device_d *imx27_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)IMX_SPI1_BASE, 0, pdata);
}

static inline struct device_d *imx27_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)IMX_I2C1_BASE, 0, pdata);
}

static inline struct device_d *imx27_add_uart0(void)
{
	return imx_add_uart((void *)IMX_UART1_BASE, 0);
}

static inline struct device_d *imx27_add_uart1(void)
{
	return imx_add_uart((void *)IMX_UART2_BASE, 1);
}

static inline struct device_d *imx27_add_uart2(void)
{
	return imx_add_uart((void *)IMX_UART3_BASE, 2);
}

static inline struct device_d *imx27_add_uart3(void)
{
	return imx_add_uart((void *)IMX_UART4_BASE, 3);
}

static inline struct device_d *imx27_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)IMX_NFC_BASE, pdata);
}

static inline struct device_d *imx27_add_fb(struct imx_fb_platform_data *pdata)
{
	return imx_add_fb((void *)0x10021000, pdata);
}

static inline struct device_d *imx27_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec((void *)IMX_FEC_BASE, pdata);
}

static inline struct device_d *imx27_add_mmc0(void *pdata)
{
	return imx_add_mmc((void *)0x10014000, 0, pdata);
}


