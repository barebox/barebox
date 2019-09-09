/*
 * Copyright (C) 2010 Sascha Hauer, Pengutronix
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
#include <platform_data/eth-fec.h>
#include <notifier.h>
#include <partition.h>
#include <gpio.h>
#include <fs.h>
#include <envfs.h>
#include <fcntl.h>
#include <nand.h>
#include <command.h>
#include <spi/spi.h>
#include <usb/ulpi.h>

#include <io.h>
#include <asm/mmu.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>

#include <mach/spi.h>
#include <mach/imx27-regs.h>
#include <mach/iomux-mx27.h>
#include <mach/imx-nand.h>
#include <mach/imx-pll.h>
#include <mach/imxfb.h>
#include <mach/devices-imx27.h>

/* two pins are controlling the CS signals to the USB phys */
#define USBH2_PHY_CS_GPIO (GPIO_PORTF + 20)
#define OTG_PHY_CS_GPIO (GPIO_PORTF + 19)

/* two pins are controlling the display and its backlight */
#define LCD_POWER_GPIO (GPIO_PORTF + 18)
#define BACKLIGHT_POWER_GPIO (GPIO_PORTE + 5)

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = 31,
};

static struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct fb_videomode imxfb_mode = {
	.name		= "CPT CLAA070LC0JCT",
	.refresh	= 60,
	.xres		= 800,
	.yres		= 480,
	.pixclock	= KHZ2PICOS(27000),
	.hsync_len	= 1,	/* DE only sync */
	.left_margin	= 50,
	.right_margin	= 50,
	.vsync_len	= 1,	/* DE only sync */
	.upper_margin	= 10,
	.lower_margin	= 10,
};

static void neso_fb_enable(int enable)
{
	gpio_direction_output(LCD_POWER_GPIO, enable);
	gpio_direction_output(BACKLIGHT_POWER_GPIO, enable);
}

static struct imx_fb_platform_data neso_fb_data = {
	.mode	= &imxfb_mode,
	.num_modes = 1,
	.pwmr	= 0x00000000,	/* doesn't matter */
	.lscr1	= 0x00120300,	/* doesn't matter */
	/* dynamic mode -> using the reset values (as recommended in the datasheet) */
	.dmacr	= (0 << 31) | (4 << 16) | 96,
	.enable	= neso_fb_enable,
	.framebuffer_ovl = (void *)0xa7f00000,
	/*
	 * - TFT style panel
	 * - clk enabled while idle
	 * - clock inverted
	 * - data not inverted
	 * - data enable high active
	 */
	.pcr = PCR_TFT |
		PCR_COLOR |
		PCR_PBSIZ_8 |
		PCR_BPIX_16 |
		PCR_CLKPOL |
		PCR_SCLK_SEL |
		PCR_LPPOL |
		PCR_FLMPOL,
	.bpp = 16,	/* TODO 32 bit does not work: The 'green' component is lacking in this mode */
};

#if defined(CONFIG_USB) && defined(CONFIG_USB_ULPI)
static void neso_usbh_init(void)
{
	uint32_t temp;

	temp = readl(MX27_USB_OTG_BASE_ADDR + 0x600);
	temp &= ~((3 << 21) | 1);
	temp |= (1 << 5) | (1 << 16) | (1 << 19) | (1 << 20) | (1<<11);
	writel(temp, MX27_USB_OTG_BASE_ADDR + 0x600);

	temp = readl(MX27_USB_OTG_BASE_ADDR + 0x584);
	temp &= ~(3 << 30);
	temp |= 2 << 30;
	writel(temp, MX27_USB_OTG_BASE_ADDR + 0x584);

	mdelay(10);

	gpio_set_value(USBH2_PHY_CS_GPIO, 0);
	mdelay(10);
	ulpi_setup((void *)(MX27_USB_OTG_BASE_ADDR + 0x570), 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC,
				    MX27_USB_OTG_BASE_ADDR + 0x400, NULL);
}
#else
static void neso_usbh_init(void) { }
#endif

static int neso_devices_init(void)
{
	int i;

	unsigned int mode[] = {
		/* UART1 */
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		/* FEC */
		PD0_AIN_FEC_TXD0,
		PD1_AIN_FEC_TXD1,
		PD2_AIN_FEC_TXD2,
		PD3_AIN_FEC_TXD3,
		PD4_AOUT_FEC_RX_ER,
		PD5_AOUT_FEC_RXD1,
		PD6_AOUT_FEC_RXD2,
		PD7_AOUT_FEC_RXD3,
		PD8_AF_FEC_MDIO,
		PD9_AIN_FEC_MDC,
		PD10_AOUT_FEC_CRS,
		PD11_AOUT_FEC_TX_CLK,
		PD12_AOUT_FEC_RXD0,
		PD13_AOUT_FEC_RX_DV,
		PD14_AOUT_FEC_RX_CLK,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,

		/* SSI1 connected in AC97 style */
		PC20_PF_SSI1_FS,
		PC21_PF_SSI1_RXD,
		PC22_PF_SSI1_TXD,
		PC23_PF_SSI1_CLK,

		/* LED 1 */
		(GPIO_PORTB | 15 | GPIO_GPIO | GPIO_OUT),
		/* LED 2 */
		(GPIO_PORTB | 16 | GPIO_GPIO | GPIO_OUT),
		/* CTOUCH reset */
		(GPIO_PORTB | 17 | GPIO_GPIO | GPIO_OUT),
		/* CTOUCH IRQ */
		(GPIO_PORTB | 14 | GPIO_GPIO | GPIO_IN),
		/* RTC IRQ */
		(GPIO_PORTF | 14 | GPIO_GPIO | GPIO_IN),
		/* SD change card detection */
		(GPIO_PORTF | 17 | GPIO_GPIO | GPIO_IN),
		/* SDHC1*/
		PE18_PF_SD1_D0,
		PE19_PF_SD1_D1,
		PE20_PF_SD1_D2,
		PE21_PF_SD1_D3,
		PE22_PF_SD1_CMD,
		PE23_PF_SD1_CLK,
		/* I2C1 */
		PD17_PF_I2C_DATA,
		PD18_PF_I2C_CLK,
		/* I2C2, for CTOUCH */
		PC5_PF_I2C2_SDA,
		PC6_PF_I2C2_SCL,

		/* Connected to: Both USB phys and ethernet phy FIXME 1 = RESET? */
		PE17_PF_RESET_OUT,

		/* USB host */
		(USBH2_PHY_CS_GPIO | GPIO_GPIO | GPIO_OUT),
		PA0_PF_USBH2_CLK,
		PA1_PF_USBH2_DIR,
		PA3_PF_USBH2_NXT,
		PA4_PF_USBH2_STP,
		PD22_AF_USBH2_DATA0,
		PD24_AF_USBH2_DATA1,
		PD23_AF_USBH2_DATA2,
		PD20_AF_USBH2_DATA3,
		PD19_AF_USBH2_DATA4,
		PD26_AF_USBH2_DATA5,
		PD21_AF_USBH2_DATA6,
		PA2_PF_USBH2_DATA7,

		/* USB OTG */
		(OTG_PHY_CS_GPIO | GPIO_GPIO | GPIO_OUT),
		PE24_PF_USBOTG_CLK,
		PE2_PF_USBOTG_DIR,
		PE0_PF_USBOTG_NXT,
		PE1_PF_USBOTG_STP,
		PC9_PF_USBOTG_DATA0,
		PC11_PF_USBOTG_DATA1,
		PC10_PF_USBOTG_DATA2,
		PC13_PF_USBOTG_DATA3,
		PC12_PF_USBOTG_DATA4,
		PC7_PF_USBOTG_DATA5,
		PC8_PF_USBOTG_DATA6,
		PE25_PF_USBOTG_DATA7,

		/* Display signals */
		(LCD_POWER_GPIO | GPIO_GPIO | GPIO_OUT), /* LCD power: 1 = LCD on */
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
		PA31_PF_OE_ACD,	/* DE */

		/* Backlight PWM (Use as gpio) */
		(BACKLIGHT_POWER_GPIO | GPIO_GPIO | GPIO_OUT),
	};

	/* reset the chip select lines to the USB/OTG phys to avoid any hang */
	gpio_direction_output(OTG_PHY_CS_GPIO, 1);
	gpio_direction_output(USBH2_PHY_CS_GPIO, 1);

	/* initialize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx27_gpio_mode(mode[i]);

	imx27_add_nand(&nand_info);
	imx27_add_fb(&neso_fb_data);

	neso_usbh_init();

	imx27_add_fec(&fec_info);

	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x80000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	armlinux_set_architecture(MACH_TYPE_NESO);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_guf_neso);

	return 0;
}

device_initcall(neso_devices_init);

static int neso_console_init(void)
{
	barebox_set_model("Garz & Fricke NESO");
	barebox_set_hostname("neso");

	imx27_add_uart0();

	return 0;
}

console_initcall(neso_console_init);

extern void *neso_pll_init, *neso_pll_init_end;

static int neso_pll(void)
{
	void *vram = (void *)0xffff4c00;
	void (*pllfunc)(void) = vram;

	printf("initialising PLLs\n");

	memcpy(vram, &neso_pll_init, 0x100);

	console_flush();

	pllfunc();

	/* clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR);

	writel(0x130410c3, MX27_CCM_BASE_ADDR + MX27_PCDR0);
	writel(0x09030911, MX27_CCM_BASE_ADDR + MX27_PCDR1);

	/* Clocks have changed. Notify clients */
	clock_notifier_call_chain();

	return 0;
}

late_initcall(neso_pll);

