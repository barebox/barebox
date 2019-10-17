/*
 * (C) 2011 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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

#define pr_fmt(fmt) "tx25: " fmt

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <environment.h>
#include <mach/imx25-regs.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <partition.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <mach/iomux-mx25.h>
#include <mach/generic.h>
#include <mach/iim.h>
#include <linux/err.h>
#include <mach/devices-imx25.h>
#include <mach/bbu.h>
#include <asm/mmu.h>

#define TX25_FEC_PWR_GPIO	IMX_GPIO_NR(4, 9)
#define TX25_FEC_RST_GPIO	IMX_GPIO_NR(4, 7)

static struct gpio fec_gpios[] = {
	{
		.gpio = TX25_FEC_PWR_GPIO,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "fec-pwr",
	}, {
		.gpio = TX25_FEC_RST_GPIO,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "fec-rst",
	},
};

static void noinline gpio_fec_active(void)
{
	int ret;

	/* power down phy, put into reset */
	ret = gpio_request_array(fec_gpios, ARRAY_SIZE(fec_gpios));
	if (ret) {
		pr_err("Failed to request fec gpios: %s\n", strerror(-ret));
		return;
	}

	udelay(10);

	/* power up phy, but leave in reset */
	gpio_set_value(TX25_FEC_PWR_GPIO, 1);

	udelay(100);

	/* FEC driver picks up the reset gpio later */
	gpio_free(TX25_FEC_RST_GPIO);
}

static int tx25_init(void)
{
	if (!of_machine_is_compatible("karo,imx25-tx25"))
		return 0;

	gpio_fec_active();

	barebox_set_hostname("tx25");
	armlinux_set_architecture(MACH_TYPE_TX25);
	armlinux_set_serial(imx_uid());

	imx_bbu_external_nand_register_handler("nand", "/dev/nand0.boot",
			BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}

console_initcall(tx25_init);

static iomux_v3_cfg_t tx25_lcdc_gpios[] = {
	MX25_PAD_A18__GPIO_2_4,		/* LCD Reset (active LOW) */
	MX25_PAD_PWM__GPIO_1_26,	/* LCD Backlight brightness 0: full 1: off */
	MX25_PAD_A19__GPIO_2_5,		/* LCD Power Enable 0: off 1: on */
	MX25_PAD_LSCLK__LSCLK,
	MX25_PAD_LD0__LD0,
	MX25_PAD_LD1__LD1,
	MX25_PAD_LD2__LD2,
	MX25_PAD_LD3__LD3,
	MX25_PAD_LD4__LD4,
	MX25_PAD_LD5__LD5,
	MX25_PAD_LD6__LD6,
	MX25_PAD_LD7__LD7,
	MX25_PAD_LD8__LD8,
	MX25_PAD_LD9__LD9,
	MX25_PAD_LD10__LD10,
	MX25_PAD_LD11__LD11,
	MX25_PAD_LD12__LD12,
	MX25_PAD_LD13__LD13,
	MX25_PAD_LD14__LD14,
	MX25_PAD_LD15__LD15,
	MX25_PAD_D15__LD16,
	MX25_PAD_D14__LD17,
	MX25_PAD_HSYNC__HSYNC,
	MX25_PAD_VSYNC__VSYNC,
	MX25_PAD_OE_ACD__OE_ACD,
};

static struct fb_videomode stk5_fb_mode = {
	.name = "G-ETV570G0DMU",
	.pixclock	= 33333,

	.xres		= 640,
	.yres		= 480,

	.hsync_len	= 64,
	.left_margin	= 96,
	.right_margin	= 80,

	.vsync_len	= 3,
	.upper_margin	= 46,
	.lower_margin	= 39,
};

#define STK5_LCD_BACKLIGHT_GPIO		IMX_GPIO_NR(1, 26)
#define STK5_LCD_RESET_GPIO		IMX_GPIO_NR(2, 4)
#define STK5_LCD_POWER_GPIO		IMX_GPIO_NR(2, 5)

static void tx25_fb_enable(int enable)
{
	if (enable) {
		gpio_direction_output(STK5_LCD_RESET_GPIO, 1);
		gpio_direction_output(STK5_LCD_POWER_GPIO, 1);
		mdelay(300);
		gpio_direction_output(STK5_LCD_BACKLIGHT_GPIO, 0);
	} else {
		gpio_direction_output(STK5_LCD_BACKLIGHT_GPIO, 1);
		gpio_direction_output(STK5_LCD_RESET_GPIO, 0);
		gpio_direction_output(STK5_LCD_POWER_GPIO, 0);
	}
}

static struct imx_fb_platform_data tx25_fb_data = {
	.mode		= &stk5_fb_mode,
	.num_modes	= 1,
	.dmacr		= 0x80040060,
	.enable		= tx25_fb_enable,
	.bpp		= 16,
	.pcr		= PCR_TFT | PCR_COLOR | PCR_FLMPOL | PCR_LPPOL | PCR_SCLK_SEL,
};

static int tx25_init_fb(void)
{
	if (!IS_ENABLED(CONFIG_DRIVER_VIDEO_IMX))
		return 0;

	if (!of_machine_is_compatible("karo,imx25-tx25"))
		return 0;

	tx25_fb_enable(0);

	mxc_iomux_v3_setup_multiple_pads(tx25_lcdc_gpios,
			ARRAY_SIZE(tx25_lcdc_gpios));

	imx25_add_fb(&tx25_fb_data);

	return 0;
}
device_initcall(tx25_init_fb);
