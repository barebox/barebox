
#include <mach/devices.h>

static inline struct device_d *imx35_add_i2c0(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)IMX_I2C1_BASE, 0, pdata);
}

static inline struct device_d *imx35_add_i2c1(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)IMX_I2C2_BASE, 1, pdata);
}

static inline struct device_d *imx35_add_i2c2(struct i2c_platform_data *pdata)
{
	return imx_add_i2c((void *)IMX_I2C3_BASE, 2, pdata);
}

static inline struct device_d *imx35_add_uart0(void)
{
	return imx_add_uart((void *)IMX_UART1_BASE, 0);
}

static inline struct device_d *imx35_add_uart1(void)
{
	return imx_add_uart((void *)IMX_UART2_BASE, 1);
}

static inline struct device_d *imx35_add_nand(struct imx_nand_platform_data *pdata)
{
	return imx_add_nand((void *)IMX_NFC_BASE, pdata);
}

static inline struct device_d *imx35_add_fb(struct imx_ipu_fb_platform_data *pdata)
{
	return imx_add_ipufb((void *)IMX_IPU_BASE, pdata);
}

static inline struct device_d *imx35_add_fec(struct fec_platform_data *pdata)
{
	return imx_add_fec((void *)IMX_FEC_BASE, pdata);
}

static inline struct device_d *imx35_add_mmc0(void *pdata)
{
	return imx_add_esdhc((void *)IMX_SDHC1_BASE, 0, pdata);
}

static inline struct device_d *imx35_add_mmc1(void *pdata)
{
	return imx_add_esdhc((void *)IMX_SDHC2_BASE, 1, pdata);
}

static inline struct device_d *imx35_add_mmc2(void *pdata)
{
	return imx_add_esdhc((void *)IMX_SDHC3_BASE, 2, pdata);
}
