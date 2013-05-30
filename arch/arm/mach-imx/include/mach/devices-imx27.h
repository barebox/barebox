
#include <mach/devices.h>
#include <mach/imx27-regs.h>

static inline struct device_d *imx27_add_spi0(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX27_CSPI1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx27_add_spi1(struct spi_imx_master *pdata)
{
	return imx_add_spi((void *)MX27_CSPI2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx27_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX27_I2C1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx27_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)MX27_I2C2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx27_add_uart0(void)
{
	return imx_add_uart_imx21((void *)MX27_UART1_BASE_ADDR, 0);
}

static inline struct device_d *imx27_add_uart1(void)
{
	return imx_add_uart_imx21((void *)MX27_UART2_BASE_ADDR, 1);
}

static inline struct device_d *imx27_add_uart2(void)
{
	return imx_add_uart_imx21((void *)MX27_UART3_BASE_ADDR, 2);
}

static inline struct device_d *imx27_add_uart3(void)
{
	return imx_add_uart_imx21((void *)MX27_UART4_BASE_ADDR, 3);
}

static inline struct device_d *imx27_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)MX27_NFC_BASE_ADDR, pdata);
}

static inline struct device_d *imx27_add_fb(struct imx_fb_platform_data *pdata)
{
	return imx_add_fb((void *)MX27_LCDC_BASE_ADDR, pdata);
}

static inline struct device_d *imx27_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec_imx27((void *)MX27_FEC_BASE_ADDR, pdata);
}

static inline struct device_d *imx27_add_mmc0(void *pdata)
{
	return imx_add_mmc((void *)MX27_SDHC1_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx27_add_mmc1(void *pdata)
{
	return imx_add_mmc((void *)MX27_SDHC2_BASE_ADDR, 1, pdata);
}

static inline struct device_d *imx27_add_mmc2(void *pdata)
{
	return imx_add_mmc((void *)MX27_SDHC3_BASE_ADDR, 2, pdata);
}

static inline struct device_d *imx27_add_usbotg(void *pdata)
{
	return imx_add_usb((void *)MX27_USB_OTG_BASE_ADDR, 0, pdata);
}

static inline struct device_d *imx27_add_usbh1(void *pdata)
{
	return imx_add_usb((void *)MX27_USB_OTG_BASE_ADDR + 0x200, 1, pdata);
}

static inline struct device_d *imx27_add_usbh2(void *pdata)
{
	return imx_add_usb((void *)MX27_USB_OTG_BASE_ADDR + 0x400, 2, pdata);
}
