
#include <mach/devices.h>

static inline struct device_d *imx51_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX51_CSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX51_CSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx51_add_spi2(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX51_CSPI3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx51_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX51_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX51_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx51_add_uart0(void)
{
	return imx_add_uart((void *)MX51_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx51_add_uart1(void)
{
	return imx_add_uart((void *)MX51_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx51_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec((void *)MX51_MXC_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx51_add_mmc0(void *pdata)
{
	return imx_add_esdhc((void *)MX51_MMC_SDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_mmc1(void *pdata)
{
	return imx_add_esdhc((void *)MX51_MMC_SDHC2_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)MX51_NFC_AXI_BASE_ADDR, pdata);
}

