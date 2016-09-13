
#include <mach/devices.h>
#include <mach/imx50-regs.h>

static inline struct device_d *imx50_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX50_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx50_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX50_ECSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx50_add_cspi(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx35((void *)MX50_CSPI_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx50_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX50_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx50_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX50_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx50_add_i2c2(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX50_I2C3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx50_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX50_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx50_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX50_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx50_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX50_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx50_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX50_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx50_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX50_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx50_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX50_ESDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx50_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX50_ESDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx50_add_mmc2(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX50_ESDHC3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx50_add_mmc3(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX50_ESDHC4_BASE_ADDR, 3, pdata);
}

static inline struct device_d *imx50_add_kpp(struct matrix_keymap_data *pdata)
{
	return imx_add_kpp((void *)MX50_KPP_BASE_ADDR, pdata);
}
