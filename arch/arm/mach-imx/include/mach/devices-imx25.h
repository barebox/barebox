
#include <mach/devices.h>
#include <mach/imx25-regs.h>

static inline struct device_d *imx25_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX25_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx25_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX25_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx25_add_i2c2(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX25_I2C3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx25_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX25_CSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx25_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX25_CSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx25_add_spi2(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX25_CSPI3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx25_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX25_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx25_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX25_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx25_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX25_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx25_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX25_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx25_add_uart4(void)
{
	return imx_add_uart_imx21((void *)MX25_UART5_BASE_ADDR, 4);
}

static inline struct device_d *imx25_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)MX25_NFC_BASE_ADDR, pdata);
}

static inline struct device_d *imx25_add_fb(struct imx_fb_platform_data *pdata)
{
	return imx_add_fb((void *)MX25_LCDC_BASE_ADDR, pdata);
}

static inline struct device_d *imx25_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX25_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx25_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX25_ESDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx25_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX25_ESDHC2_BASE_ADDR, 1, pdata);
}
