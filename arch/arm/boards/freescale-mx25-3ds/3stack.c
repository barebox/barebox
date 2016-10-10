/*
 * (C) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <environment.h>
#include <mach/imx25-regs.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <envfs.h>
#include <partition.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <platform_data/eth-fec.h>
#include <nand.h>
#include <mach/iomux-mx25.h>
#include <mach/generic.h>
#include <mach/iim.h>
#include <linux/err.h>
#include <i2c/i2c.h>
#include <mfd/mc34704.h>
#include <mach/devices-imx25.h>
#include <asm/barebox-arm-head.h>

static struct fec_platform_data fec_info = {
	.xcv_type	= PHY_INTERFACE_MODE_RMII,
	.phy_addr	= 1,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
};

#ifdef CONFIG_USB
static void imx25_usb_init(void)
{
	unsigned int tmp;

	/* Host 2 */
	tmp = readl(MX25_USB_OTG_BASE_ADDR + 0x600);
	tmp &= ~(3 << 21);
	tmp |= (2 << 21) | (1 << 4) | (1 << 5);
	writel(tmp, MX25_USB_OTG_BASE_ADDR + 0x600);

	tmp = readl(MX25_USB_OTG_BASE_ADDR + 0x584);
	tmp |= 3 << 30;
	writel(tmp, MX25_USB_OTG_BASE_ADDR + 0x584);

	/* Set to Host mode */
	tmp = readl(MX25_USB_OTG_BASE_ADDR + 0x5a8);
	writel(tmp | 0x3, MX25_USB_OTG_BASE_ADDR + 0x5a8);
}
#endif

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("mc34704", 0x54),
	},
};

static int imx25_3ds_pmic_init(void)
{
	struct mc34704 *pmic;

	pmic = mc34704_get();
	if (pmic == NULL)
		return -EIO;

	return mc34704_reg_write(pmic, 0x2, 0x9);
}

static int imx25_3ds_fec_init(void)
{
	int ret;

	ret = imx25_3ds_pmic_init();
	if (ret < 0)
		return ret;

	/*
	 * Set up the FEC_RESET_B and FEC_ENABLE GPIO pins.
	 * Assert FEC_RESET_B, then power up the PHY by asserting
	 * FEC_ENABLE, at the same time lifting FEC_RESET_B.
	 *
	 * FEC_RESET_B: gpio2[3] is ALT 5 mode of pin A17
	 * FEC_ENABLE_B: gpio4[8] is ALT 5 mode of pin D12
	 */
	writel(0x8, MX25_IOMUXC_BASE_ADDR + 0x0238); /* open drain */
	writel(0x0, MX25_IOMUXC_BASE_ADDR + 0x028C); /* cmos, no pu/pd */

#define FEC_ENABLE_GPIO		35
#define FEC_RESET_B_GPIO	104

	/* make the pins output */
	gpio_direction_output(FEC_ENABLE_GPIO, 0);  /* drop PHY power */
	gpio_direction_output(FEC_RESET_B_GPIO, 0); /* assert reset */
	udelay(2);

	/* turn on power & lift reset */
	gpio_set_value(FEC_ENABLE_GPIO, 1);
	gpio_set_value(FEC_RESET_B_GPIO, 1);

	return 0;
}
late_initcall(imx25_3ds_fec_init);

static int imx25_3ds_devices_init(void)
{
#ifdef CONFIG_USB
	/* USB does not work yet. Don't know why. Maybe
	 * the CPLD has to be initialized.
	 */
	imx25_usb_init();
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX25_USB_OTG_BASE_ADDR + 0x400, NULL);
#endif

	imx25_iim_register_fec_ethaddr();
	imx25_add_fec(&fec_info);

	add_mem_device("sram0", 0x78000000, 128 * 1024, IORESOURCE_MEM_WRITEABLE);

	if (readl(MX25_CCM_BASE_ADDR + MX25_CCM_RCSR) & (1 << 14))
		nand_info.width = 2;

	imx25_add_nand(&nand_info);

	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	imx25_add_i2c0(NULL);

	armlinux_set_architecture(MACH_TYPE_MX25_3DS);
	armlinux_set_serial(imx_uid());

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_mx25_3ds);

	return 0;
}

device_initcall(imx25_3ds_devices_init);

static iomux_v3_cfg_t imx25_pads[] = {
	MX25_PAD_FEC_MDC__FEC_MDC,
	MX25_PAD_FEC_MDIO__FEC_MDIO,
	MX25_PAD_FEC_RDATA0__FEC_RDATA0,
	MX25_PAD_FEC_RDATA1__FEC_RDATA1,
	MX25_PAD_FEC_RX_DV__FEC_RX_DV,
	MX25_PAD_FEC_TDATA0__FEC_TDATA0,
	MX25_PAD_FEC_TDATA1__FEC_TDATA1,
	MX25_PAD_FEC_TX_CLK__FEC_TX_CLK,
	MX25_PAD_FEC_TX_EN__FEC_TX_EN,
	MX25_PAD_POWER_FAIL__POWER_FAIL,
	MX25_PAD_A17__GPIO_2_3,
	MX25_PAD_D12__GPIO_4_8,
	/* UART1 */
	MX25_PAD_UART1_RXD__UART1_RXD,
	MX25_PAD_UART1_TXD__UART1_TXD,
	MX25_PAD_UART1_RTS__UART1_RTS,
	MX25_PAD_UART1_CTS__UART1_CTS,
	/* USBH2 */
	MX25_PAD_D9__USBH2_PWR,
	MX25_PAD_D8__USBH2_OC,
	MX25_PAD_LD0__USBH2_CLK,
	MX25_PAD_LD1__USBH2_DIR,
	MX25_PAD_LD2__USBH2_STP,
	MX25_PAD_LD3__USBH2_NXT,
	MX25_PAD_LD4__USBH2_DATA0,
	MX25_PAD_LD5__USBH2_DATA1,
	MX25_PAD_LD6__USBH2_DATA2,
	MX25_PAD_LD7__USBH2_DATA3,
	MX25_PAD_HSYNC__USBH2_DATA4,
	MX25_PAD_VSYNC__USBH2_DATA5,
	MX25_PAD_LSCLK__USBH2_DATA6,
	MX25_PAD_OE_ACD__USBH2_DATA7,
	/* i2c */
	MX25_PAD_I2C1_CLK__I2C1_CLK,
	MX25_PAD_I2C1_DAT__I2C1_DAT,
};

static int imx25_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(imx25_pads, ARRAY_SIZE(imx25_pads));

	writel(0x03010101, 0x53f80024);

	barebox_set_model("Freescale i.MX25 3DS");
	barebox_set_hostname("mx25-3stack");

	imx25_add_uart0();
	return 0;
}

console_initcall(imx25_console_init);

static int imx25_core_setup(void)
{
	writel(0x01010103, MX25_CCM_BASE_ADDR + MX25_CCM_PCDR2);
	return 0;

}
core_initcall(imx25_core_setup);
