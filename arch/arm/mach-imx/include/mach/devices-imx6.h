#include <mach/devices.h>

static inline struct device_d *imx6_add_uart0(void)
{
	return imx_add_uart((void *)MX6_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx6_add_uart1(void)
{
	return imx_add_uart((void *)MX6_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx6_add_uart2(void)
{
	return imx_add_uart((void *)MX6_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx6_add_uart3(void)
{
	return imx_add_uart((void *)MX6_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx6_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX6_USDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx6_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX6_USDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx6_add_mmc2(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX6_USDHC3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx6_add_mmc3(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX6_USDHC4_BASE_ADDR, 3, pdata);
}

static inline struct device_d *imx6_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec((void *)MX6_ENET_BASE_ADDR, pdata);
}

static inline struct device_d *imx6_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx6_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX6_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx6_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX6_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx6_add_i2c2(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX6_I2C3_BASE_ADDR, 2, pdata);
}
