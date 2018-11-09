
#include <linux/sizes.h>
#include <mach/devices.h>
#include <mach/imx51-regs.h>

static inline struct device_d *imx51_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX51_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx51((void *)MX51_ECSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx51_add_cspi(struct spi_imx_master *pdata)
{
	return imx_add_spi_imx35((void *)MX51_CSPI_BASE_ADDR, 2, pdata);
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
	return imx_add_uart_imx21((void *)MX51_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx51_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX51_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx51_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX51_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx51_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX51_MXC_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx51_add_mmc0(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX51_MMC_SDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_mmc1(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX51_MMC_SDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx51_add_mmc2(struct esdhc_platform_data *pdata)
{
	return imx_add_esdhc((void *)MX51_MMC_SDHC3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx51_add_nand(struct imx_nand_platform_data *pdata)
{
	struct resource res[] = {
		{
			.start = MX51_NFC_BASE_ADDR,
			.end = MX51_NFC_BASE_ADDR + SZ_4K - 1,
			.flags = IORESOURCE_MEM,
		}, {
			.start = MX51_NFC_AXI_BASE_ADDR,
			.end = MX51_NFC_AXI_BASE_ADDR + SZ_4K - 1,
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

static inline struct device_d *imx51_add_kpp(struct matrix_keymap_data *pdata)
{
	return imx_add_kpp((void *)MX51_KPP_BASE_ADDR, pdata);
}

static inline struct device_d *imx51_add_pata(void)
{
	return imx_add_pata((void *)MX51_ATA_BASE_ADDR);
}

static inline struct device_d *imx51_add_usbotg(void *pdata)
{
	return imx_add_usb((void *)MX51_OTG_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx51_add_usbh1(void *pdata)
{
	return imx_add_usb((void *)MX51_OTG_BASE_ADDR + 0x200, 1, pdata);
}

static inline struct device_d *imx51_add_usbh2(void *pdata)
{
	return imx_add_usb((void *)MX51_OTG_BASE_ADDR + 0x400, 2, pdata);
}
