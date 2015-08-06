/*
 * (C) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *     2010 by Marc Kleine-Budde <kernel@pengutronix.de>
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
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <init.h>
#include <partition.h>
#include <linux/sizes.h>

#include <gpio.h>
#include <mach/mfp-pxa27x.h>
#include <mach/pxa-regs.h>
#include <mach/pxafb.h>
#include <mach/devices.h>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <generated/mach-types.h>
#include <asm/mmu.h>

#define PCM990_CTRL_PHYS	(void *)PXA_CS1_PHYS

#define PCM990_CTRL_REG3	0x0006	/* LCD CTRL REGISTER 3 */
#define PCM990_CTRL_LCDPWR	0x0001	/* RW LCD Power on */
#define PCM990_CTRL_LCDON	0x0002	/* RW LCD Latch on */
#define PCM990_CTRL_LCDPOS1	0x0004	/* RW POS 1 */
#define PCM990_CTRL_LCDPOS2	0x0008	/* RW POS 2 */

static void lcd_power(int on)
{
	void __iomem *ctrl3 = PCM990_CTRL_PHYS + PCM990_CTRL_REG3;

	if (on)
		writeb(PCM990_CTRL_LCDPWR | PCM990_CTRL_LCDON, ctrl3);
	else
		writeb(0x0, ctrl3);
}

static void backlight_power(int on)
{
	if (on) {
		mdelay(20);
		gpio_set_value(16, 1);
	} else {
		gpio_set_value(16, 0);
	}
}

static struct pxafb_videomode pxafb_mode = {
	.mode = {
		.pixclock	= 28000,
		.xres		= 640,
		.yres		= 480,
		.hsync_len	= 20,
		.left_margin	= 103,
		.right_margin	= 47,
		.vsync_len	= 6,
		.upper_margin	= 28,
		.lower_margin	= 5,
		.sync		= 0,
	},
	.bpp			= 16,
};

static struct pxafb_platform_data fb_pdata = {
	.mode			= &pxafb_mode,
	.lcd_conn		= LCD_COLOR_TFT_16BPP | LCD_PCLK_EDGE_FALL,
	.lcd_power		= lcd_power,
	.backlight_power	= backlight_power,
};

static int pcm027_mem_init(void)
{
	arm_add_mem_device("ram0", 0xa0000000, SZ_64M);

	return 0;
}
mem_initcall(pcm027_mem_init);

static unsigned long pin_config[] = {
	/* Chip Selects */
	GPIO20_nSDCS_2,
	GPIO21_nSDCS_3,
	GPIO15_nCS_1,
	GPIO78_nCS_2,
	GPIO80_nCS_4,

	/* Variable Latency I/O Ready Pin */
	GPIO18_RDY,

	/* FFUART */
	GPIO85_nPCE_1,		/* enables RX */
	GPIO34_FFUART_RXD,
	GPIO35_FFUART_CTS,
	GPIO36_FFUART_DCD,
	GPIO37_FFUART_DSR,
	GPIO38_FFUART_RI,
	GPIO39_FFUART_TXD,
	GPIO40_FFUART_DTR,
	GPIO41_FFUART_RTS,

	/* LCD */
	GPIO58_LCD_LDD_0,
	GPIO59_LCD_LDD_1,
	GPIO60_LCD_LDD_2,
	GPIO61_LCD_LDD_3,
	GPIO62_LCD_LDD_4,
	GPIO63_LCD_LDD_5,
	GPIO64_LCD_LDD_6,
	GPIO65_LCD_LDD_7,
	GPIO66_LCD_LDD_8,
	GPIO67_LCD_LDD_9,
	GPIO68_LCD_LDD_10,
	GPIO69_LCD_LDD_11,
	GPIO70_LCD_LDD_12,
	GPIO71_LCD_LDD_13,
	GPIO72_LCD_LDD_14,
	GPIO73_LCD_LDD_15,
	GPIO74_LCD_FCLK,
	GPIO75_LCD_LCLK,
	GPIO76_LCD_PCLK,
	GPIO77_LCD_BIAS,
	MFP_CFG_OUT(GPIO16, AF0, DRIVE_LOW),	/* backlight */

	/* NIC */
	GPIO33_nCS_5,
	GPIO49_nPWE,
};

static int pcm027_devices_init(void)
{
	void *cfi_iospace;

	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL, 0x14000300, 16,
			IORESOURCE_MEM, NULL);

	cfi_iospace = map_io_sections(0x0, (void *)0xe0000000, SZ_32M);
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, (unsigned long)cfi_iospace, SZ_32M, 0);

	pxa_add_fb((void *)0x44000000, &fb_pdata);

	armlinux_set_architecture(MACH_TYPE_PCM027);

	devfs_add_partition("nor0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", SZ_512K, SZ_256K, DEVFS_PARTITION_FIXED, "env0");
	protect_file("/dev/env0", 1);

	return 0;
}

device_initcall(pcm027_devices_init);

static int pcm027_console_init(void)
{
	/* route pins */
	pxa2xx_mfp_config(ARRAY_AND_SIZE(pin_config));

	/* enable clock */
	CKEN |= CKEN_FFUART;

	barebox_set_model("Phytec phyCORE-PXA270");
	barebox_set_hostname("pcm027");

	pxa_add_uart((void *)0x40100000, 0);

	return 0;
}

console_initcall(pcm027_console_init);
