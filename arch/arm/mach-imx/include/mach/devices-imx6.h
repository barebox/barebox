#include <mach/devices.h>
#include <mach/imx6-regs.h>

static inline struct device_d *imx6_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX6_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx6_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX6_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx6_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX6_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx6_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX6_UART4_BASE_ADDR, 3);
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
	return imx_add_fec_imx6((void *)MX6_ENET_BASE_ADDR, pdata);
}

static inline struct device_d *imx6_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx6_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx6_add_spi2(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx6_add_spi3(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI4_BASE_ADDR, 3, pdata);
}

static inline struct device_d *imx6_add_spi4(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX6_ECSPI5_BASE_ADDR, 4, pdata);
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

static inline struct device_d *imx6_add_sata(void)
{
	return add_generic_device("imx6-sata", 0, NULL, MX6_SATA_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
}

static inline struct device_d *imx6_add_usbotg(void *pdata)
{
	add_generic_device("imx-usb-phy", 0, NULL, MX6_USBPHY1_BASE_ADDR, 0x1000,
			IORESOURCE_MEM, NULL);

	return imx_add_usb((void *)MX6_USBOH3_USB_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx6_add_usbh1(void *pdata)
{
	add_generic_device("imx-usb-phy", 1, NULL, MX6_USBPHY2_BASE_ADDR, 0x1000,
			IORESOURCE_MEM, NULL);

	return imx_add_usb((void *)MX6_USBOH3_USB_BASE_ADDR + 0x200, 1, pdata);
}

static inline struct device_d *imx6_add_usbh2(void *pdata)
{
	return imx_add_usb((void *)MX6_USBOH3_USB_BASE_ADDR + 0x400, 2, pdata);
}

static inline struct device_d *imx6_add_usbh3(void *pdata)
{
	return imx_add_usb((void *)MX6_USBOH3_USB_BASE_ADDR + 0x600, 2, pdata);
}
