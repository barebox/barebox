
#include <mach/devices.h>
#include <mach/imx53-regs.h>

static inline struct device_d *imx53_add_cspi(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx35((void *)MX53_CSPI_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX53_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX53_ECSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx53_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX53_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX53_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx53_add_i2c2(struct i2c_platform_data *pdata)
{
       return imx_add_i2c((void *)MX53_I2C3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx53_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX53_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx53_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX53_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx53_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX53_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx53_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX53_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx53_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX53_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx53_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx53_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx53_add_mmc2(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx53_add_mmc3(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX53_ESDHC4_BASE_ADDR, 3, pdata);
}

static inline struct device_d *imx53_add_nand(struct imx_nand_platform_data *pdata)
{
	struct resource res[] = {
		{
			.start = MX53_NFC_BASE_ADDR,
			.end = MX53_NFC_BASE_ADDR + SZ_4K - 1,
			.flags = IORESOURCE_MEM,
		}, {
			.start = MX53_NFC_AXI_BASE_ADDR,
			.end = MX53_NFC_AXI_BASE_ADDR + SZ_4K - 1,
			.flags = IORESOURCE_MEM,
		},
	};
	struct device_d *dev = xzalloc(sizeof(*dev));

	dev->resource = xzalloc(sizeof(struct resource) * ARRAY_SIZE(res));
	memcpy(dev->resource, res, sizeof(struct resource) * ARRAY_SIZE(res));
	dev->num_resources = ARRAY_SIZE(res);
	dev_set_name(dev, "imx_nand");
	dev->id = DEVICE_ID_DYNAMIC;
	dev->platform_data = pdata;

	platform_device_register(dev);

	return dev;
}

static inline struct device_d *imx53_add_kpp(struct matrix_keymap_data *pdata)
{
	return imx_add_kpp((void *)MX53_KPP_BASE_ADDR, pdata);
}

static inline struct device_d *imx53_add_sata(void)
{
	return add_generic_device("imx53-sata", 0, NULL, MX53_SATA_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
}
