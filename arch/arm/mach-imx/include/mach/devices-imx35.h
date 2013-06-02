
#include <mach/devices.h>
#include <mach/imx35-regs.h>

static inline struct device_d *imx35_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX35_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx35_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX35_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx35_add_i2c2(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX35_I2C3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx35_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX35_CSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx35_add_spi(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX35_CSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx35_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX35_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx35_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX35_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx35_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX35_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx35_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)MX35_NFC_BASE_ADDR, pdata);
}

static inline struct device_d *imx35_add_fb(struct imx_ipu_fb_platform_data *pdata)
{
	return imx_add_ipufb((void *)MX35_IPU_CTRL_BASE_ADDR, pdata);
}

static inline struct device_d *imx35_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX35_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx35_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX35_ESDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx35_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX35_ESDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx35_add_mmc2(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX35_ESDHC3_BASE_ADDR, 2, pdata);
}
