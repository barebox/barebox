/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix 
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
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx27-regs.h>
#include <fec.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <mach/imxfb.h>
#include <gpio.h>
#include <asm/mmu.h>
#include <usb/ulpi.h>
#include <mach/iomux-mx27.h>
#include <mach/devices-imx27.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = 1,
};

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
	.flash_bbt = 1,
};

static struct imx_fb_videomode imxfb_mode[] = {
	{
		.mode = {
			.name		= "Primeview-PD050VL1",
			.refresh	= 60,
			.xres		= 640,
			.yres		= 480,
			.pixclock	= 40000, /* in ps (25MHz) */
			.hsync_len	= 32,
			.left_margin	= 112,
			.right_margin	= 36,
			.vsync_len	= 2,
			.upper_margin	= 33,
			.lower_margin	= 33,
		},
		.pcr = 0xF0C88080,
		.bpp = 16,
	}, {
		.mode = {
			.name		= "Primeview-PD035VL1",
			.refresh	= 60,
			.xres		= 640,
			.yres		= 480,
			.pixclock	= 40000, /* in ps (25 MHz) */
			.hsync_len	= 30,
			.left_margin	= 98,
			.right_margin	= 36,
			.vsync_len	= 2,
			.upper_margin	= 15,
			.lower_margin	= 33,
		},
		.pcr = 0xF0C88080,
		.bpp = 16,
	}, {
		.mode = {
			.name		= "Primeview-PD104SLF",
			.refresh	= 60,
			.xres		= 800,
			.yres		= 600,
			.pixclock	= 25000, /* in ps (40,0 MHz) */
			.hsync_len	= 40,
			.left_margin	= 174,
			.right_margin	= 174,
			.vsync_len	= 4,
			.upper_margin	= 24,
			.lower_margin	= 23,
		},
		.pcr = 0xF0C88080,
		.bpp = 16,
	}, {
		.mode = {
			.name		= "Primeview-PM070WL4",
			.refresh	= 60,
			.xres		= 800,
			.yres		= 480,
			.pixclock	= 31250, /* in ps (32 MHz) */
			.hsync_len	= 40,
			.left_margin	= 174,
			.right_margin	= 174,
			.vsync_len	= 2,
			.upper_margin	= 33,
			.lower_margin	= 23,
		},
		.pcr = 0xF0C88080,
		.bpp = 16,
	},
};

static struct imx_fb_platform_data pca100_fb_data = {
	.mode   = imxfb_mode,
	.num_modes = ARRAY_SIZE(imxfb_mode),
	.pwmr   = 0x00A903FF,
	.lscr1  = 0x00120300,
	.dmacr  = 0x00040060,
};

#ifdef CONFIG_USB
static void pca100_usb_register(void)
{
	mdelay(10);

	gpio_direction_output(GPIO_PORTB + 24, 0);
	gpio_direction_output(GPIO_PORTB + 23, 0);

	mdelay(10);

	ulpi_setup((void *)(MX27_USB_OTG_BASE_ADDR + 0x170), 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX27_USB_OTG_BASE_ADDR, NULL);
	ulpi_setup((void *)(MX27_USB_OTG_BASE_ADDR + 0x570), 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, MX27_USB_OTG_BASE_ADDR + 0x400, NULL);
}
#endif

static void pca100_usb_init(void)
{
	u32 reg;

	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x600);
	reg &= ~((3 << 21) | 1);
	reg |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 11) | (1 << 20);
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x600);

	/*
	 * switch usbotg and usbh2 to ulpi mode. Do this *before*
	 * the iomux setup to prevent funny hardware bugs from
	 * triggering. Also, do this even when USB support is
	 * disabled to give Linux USB support a good start.
	 */
	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x584);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x584);

	reg = readl(MX27_USB_OTG_BASE_ADDR + 0x184);
	reg &= ~(3 << 30);
	reg |= 2 << 30;
	writel(reg, MX27_USB_OTG_BASE_ADDR + 0x184);

	/* disable the usb phys */
	imx_gpio_mode((GPIO_PORTB | 23) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 23, 1);
	imx_gpio_mode((GPIO_PORTB | 24) | GPIO_GPIO | GPIO_IN);
	gpio_direction_output(GPIO_PORTB + 24, 1);
}

static int pca100_devices_init(void)
{
	int i;
	struct device_d *nand;

	unsigned int mode[] = {
		PD0_AIN_FEC_TXD0,
		PD1_AIN_FEC_TXD1,
		PD2_AIN_FEC_TXD2,
		PD3_AIN_FEC_TXD3,
		PD4_AOUT_FEC_RX_ER,
		PD5_AOUT_FEC_RXD1,
		PD6_AOUT_FEC_RXD2,
		PD7_AOUT_FEC_RXD3,
		PD8_AF_FEC_MDIO,
		PD9_AIN_FEC_MDC | GPIO_PUEN,
		PD10_AOUT_FEC_CRS,
		PD11_AOUT_FEC_TX_CLK,
		PD12_AOUT_FEC_RXD0,
		PD13_AOUT_FEC_RX_DV,
		PD14_AOUT_FEC_RX_CLK,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		PD25_PF_CSPI1_RDY,
		PD26_PF_CSPI1_SS2,
		PD27_PF_CSPI1_SS1,
		PD28_PF_CSPI1_SS0,
		PD29_PF_CSPI1_SCLK,
		PD30_PF_CSPI1_MISO,
		PD31_PF_CSPI1_MOSI,
		/* USB host 2 */
		PA0_PF_USBH2_CLK,
		PA1_PF_USBH2_DIR,
		PA2_PF_USBH2_DATA7,
		PA3_PF_USBH2_NXT,
		PA4_PF_USBH2_STP,
		PD19_AF_USBH2_DATA4,
		PD20_AF_USBH2_DATA3,
		PD21_AF_USBH2_DATA6,
		PD22_AF_USBH2_DATA0,
		PD23_AF_USBH2_DATA2,
		PD24_AF_USBH2_DATA1,
		PD26_AF_USBH2_DATA5,
		/* SDHC */
		PB4_PF_SD2_D0,
		PB5_PF_SD2_D1,
		PB6_PF_SD2_D2,
		PB7_PF_SD2_D3,
		PB8_PF_SD2_CMD,
		PB9_PF_SD2_CLK,
		PC7_PF_USBOTG_DATA5,
		PC8_PF_USBOTG_DATA6,
		PC9_PF_USBOTG_DATA0,
		PC10_PF_USBOTG_DATA2,
		PC11_PF_USBOTG_DATA1,
		PC12_PF_USBOTG_DATA4,
		PC13_PF_USBOTG_DATA3,
		PE0_PF_USBOTG_NXT,
		PE1_PF_USBOTG_STP,
		PE2_PF_USBOTG_DIR,
		PE24_PF_USBOTG_CLK,
		PE25_PF_USBOTG_DATA7,
		/* display */
		PA5_PF_LSCLK,
		PA6_PF_LD0,
		PA7_PF_LD1,
		PA8_PF_LD2,
		PA9_PF_LD3,
		PA10_PF_LD4,
		PA11_PF_LD5,
		PA12_PF_LD6,
		PA13_PF_LD7,
		PA14_PF_LD8,
		PA15_PF_LD9,
		PA16_PF_LD10,
		PA17_PF_LD11,
		PA18_PF_LD12,
		PA19_PF_LD13,
		PA20_PF_LD14,
		PA21_PF_LD15,
		PA22_PF_LD16,
		PA23_PF_LD17,
		PA26_PF_PS,
		PA28_PF_HSYNC,
		PA29_PF_VSYNC,
		PA31_PF_OE_ACD,
		/* external I2C */
		PD17_PF_I2C_DATA,
		PD18_PF_I2C_CLK,
	};

	pca100_usb_init();

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	imx27_add_nand(&nand_info);
	imx27_add_fec(&fec_info);
	imx27_add_mmc1(NULL);
	imx27_add_fb(&pca100_fb_data);

#ifdef CONFIG_USB
	pca100_usb_register();
#endif

	nand = get_device_by_name("nand0");
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(2149);

	return 0;
}

device_initcall(pca100_devices_init);

static int pca100_console_init(void)
{
	barebox_set_model("Phytec phyCARD-i.MX27");
	barebox_set_hostname("phycard-imx27");

	imx27_add_uart0();
	return 0;
}

console_initcall(pca100_console_init);
