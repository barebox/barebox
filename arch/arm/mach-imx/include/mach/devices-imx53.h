
#include <mach/devices.h>

static inline struct device_d *imx53_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX53_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX53_ECSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx53_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX53_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX53_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx53_add_uart0(void)
{
	return imx_add_uart((void *)MX53_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx53_add_uart1(void)
{
	return imx_add_uart((void *)MX53_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx53_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec((void *)MX53_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx53_add_mmc0(void *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_mmc1(void *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC2_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_mmc2(void *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC3_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_nand(struct imx_nand_platform_data *pdata)
{
	struct resource res[] = {
		{
			.start = MX53_NFC_BASE_ADDR,
			.size = SZ_4K,
			.flags = IORESOURCE_MEM,
		}, {
			.start = MX53_NFC_AXI_BASE_ADDR,
			.size = SZ_4K,
			.flags = IORESOURCE_MEM,
		},
	};
	struct device_d *dev = xzalloc(sizeof(*dev));

	dev->resource = xzalloc(sizeof(struct resource) * ARRAY_SIZE(res));
	memcpy(dev->resource, res, sizeof(struct resource) * ARRAY_SIZE(res));
	dev->num_resources = ARRAY_SIZE(res);
	strcpy(dev->name, "imx_nand");
	dev->id = -1;
	dev->platform_data = pdata;

	register_device(dev);

	return dev;
}
