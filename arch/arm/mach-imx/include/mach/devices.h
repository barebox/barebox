
#include <fec.h>
#include <matrix_keypad.h>
#include <i2c/i2c.h>
#include <mach/spi.h>
#include <mach/imx-nand.h>
#include <mach/imxfb.h>
#include <mach/imx-ipu-fb.h>
#include <mach/esdhc.h>
#include <usb/chipidea-imx.h>

struct device_d *imx_add_fec_imx27(void *base, struct fec_platform_data *pdata);
struct device_d *imx_add_fec_imx6(void *base, struct fec_platform_data *pdata);
struct device_d *imx_add_spi(void *base, int id, struct spi_imx_master *pdata);
struct device_d *imx_add_i2c(void *base, int id, struct i2c_platform_data *pdata);
struct device_d *imx_add_uart_imx1(void *base, int id);
struct device_d *imx_add_uart_imx21(void *base, int id);
struct device_d *imx_add_nand(void *base, struct imx_nand_platform_data *pdata);
struct device_d *imx_add_fb(void *base, struct imx_fb_platform_data *pdata);
struct device_d *imx_add_ipufb(void *base, struct imx_ipu_fb_platform_data *pdata);
struct device_d *imx_add_mmc(void *base, int id, void *pdata);
struct device_d *imx_add_esdhc(void *base, int id, struct esdhc_platform_data *pdata);
struct device_d *imx_add_kpp(void *base, struct matrix_keymap_data *pdata);
struct device_d *imx_add_pata(void *base);
struct device_d *imx_add_usb(void *base, int id, struct imxusb_platformdata *pdata);
