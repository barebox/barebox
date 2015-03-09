/*
 * Copyright (C) 2010 Marek Belisko <marek.belisko@open-nandra.com>
 *
 * Based on a9m2440.c board init by Juergen Beisert, Pengutronix
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
#include <init.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <dm9000.h>
#include <nand.h>
#include <mci.h>
#include <fb.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <io.h>
#include <gpio.h>
#include <mach/bbu.h>
#include <mach/iomux.h>
#include <mach/s3c-iomap.h>
#include <mach/devices-s3c24xx.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-mci.h>
#include <mach/s3c24xx-fb.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(MINI2440_TACLS, MINI2440_TWRPH0,
							MINI2440_TWRPH1),
	.flash_bbt = 1,	/* same as the kernel */
};

/*
 * dm9000 network controller onboard
 * Connected to CS line 4 and interrupt line EINT7,
 * data width is 16 bit
 * Area 1: Offset 0x300...0x303
 * Area 2: Offset 0x304...0x307
 */
static struct dm9000_platform_data dm9000_data = {
	.srom     = 1,
};

static struct s3c_mci_platform_data mci_data = {
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,
	.gpio_detect = 232,	/* GPG8_GPIO */
	.detect_invert = 0,
};

static struct fb_videomode s3c24x0_fb_modes[] = {
#ifdef CONFIG_MINI2440_VIDEO_N35
	{
		.name		= "N35",
		.refresh	= 60,
		.xres		= 240,
		.left_margin	= 21,
		.right_margin	= 38,
		.hsync_len	= 6,
		.yres		= 320,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.vsync_len	= 2,
		.pixclock	= 115913,
		.sync		= FB_SYNC_USE_PWREN,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
#endif
#ifdef CONFIG_MINI2440_VIDEO_A70
	{
		.name		= "A70",
		.refresh	= 50,
		.xres		= 800,
		.left_margin	= 40,
		.right_margin	= 40,
		.hsync_len	= 48,
		.yres		= 480,
		.upper_margin	= 29,
		.lower_margin	= 3,
		.vsync_len	= 3,
		.pixclock	= 41848,
		.sync		= FB_SYNC_USE_PWREN | FB_SYNC_DE_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
#endif
#ifdef CONFIG_MINI2440_VIDEO_SVGA
	{
		.name		= "SVGA",
		.refresh	= 24,
		.xres		= 1024,
		.left_margin	= 1,
		.right_margin	= 2,
		.hsync_len	= 2,
		.yres		= 768,
		.upper_margin	= 200,
		.lower_margin	= 16,
		.vsync_len	= 16,
		.pixclock	= 40492,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT | FB_SYNC_DE_HIGH_ACT
                                  /* | FB_SYNC_SWAP_HW */ /* FIXME maybe */ ,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
#endif
#ifdef CONFIG_MINI2440_VIDEO_W35
	{
		.name		= "W35",
		.refresh	= 60,
		.xres		= 320,
		.left_margin	= 68,
		.right_margin	= 66,
		.hsync_len	= 4,
		.yres		= 240,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.vsync_len	= 9,
		.pixclock	= 115913,
		.sync		= FB_SYNC_USE_PWREN | FB_SYNC_CLK_INVERT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
#endif
};

static struct s3c_fb_platform_data s3c24x0_fb_data = {
	.mode_list		= s3c24x0_fb_modes,
	.mode_cnt		= sizeof(s3c24x0_fb_modes) / sizeof(struct fb_videomode),
	.bits_per_pixel		= 16,
	.passive_display	= 0,
};

static const unsigned pin_usage[] = {
	/* address bus, used by NOR, SDRAM */
	GPA1_ADDR16,
	GPA2_ADDR17,
	GPA3_ADDR18,
	GPA4_ADDR19,
	GPA5_ADDR20,
	GPA6_ADDR21,
	GPA7_ADDR22,

	GPA8_ADDR23_GPIO | GPIO_IN,
	GPA9_ADDR24,	/* BA0 */
	GPA10_ADDR25,	/* BA1 */
	GPA11_ADDR26_GPIO | GPIO_IN,	/* not connected */

	/* DM9000 requirements */
	GPA15_NGCS4,
	GPF7_EINT7,

	/* de-activate the speaker */
	GPB0_GPIO | GPIO_OUT | GPIO_VAL(0),

	/* SD socket */
	GPE5_SDCLK,
	GPE6_SDCMD,
	GPE7_SDDAT0,
	GPE8_SDDAT1,
	GPE9_SDDAT2,
	GPE10_SDDAT3,
	GPG8_GPIO | GPIO_IN,	/* change detection */
	GPH8_GPIO | GPIO_IN,	/* write protection sense */

	/* NAND requirements */
	GPA17_CLE,
	GPA18_ALE,
	GPA19_NFWE,
	GPA20_NFRE,
	GPA21_NRSTOUT,
	GPA22_NFCE,

	/* Video out */
	GPC0_LEND,
	GPC1_VCLK,
	GPC2_VLINE,
	GPC3_VFRAME,
	GPC4_VM,
	GPC5_LPCOE,
	GPC6_LPCREV,
	GPC7_LPCREVB,
	GPG4_LCD_PWREN,

	GPC8_VD0,
	GPC9_VD1,
	GPC10_VD2,
	GPC11_VD3,
	GPC12_VD4,
	GPC13_VD5,
	GPC14_VD6,
	GPC15_VD7,
	GPD0_VD8,
	GPD1_VD9,
	GPD2_VD10,
	GPD3_VD11,
	GPD4_VD12,
	GPD5_VD13,
	GPD6_VD14,
	GPD7_VD15,
	GPD8_VD16,
	GPD9_VD17,
	GPD10_VD18,
	GPD11_VD19,
	GPD12_VD20,
	GPD13_VD21,
	GPD14_VD22,
	GPD15_VD23,

	/* K6 or CON12, pin 6, external pull up  */
	GPG11_EINT19 | GPIO_IN,
	/* K5 or CON12, pin 5*/
	GPG7_EINT15 | GPIO_IN,
	/* K4 or CON12, pin 4 */
	GPG6_EINT14 | GPIO_IN,
	/* K3 or CON12, pin 3 */
	GPG5_EINT13 | GPIO_IN,
	/* K2 or CON12, pin 2 */
	GPG3_EINT11 | GPIO_IN,
	/* K1 or CON12, pin 1, external pull up */
	GPG0_EINT8 | GPIO_IN,

	/* LED 1 1=off */
	GPB5_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 2 1=off */
	GPB6_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 3 1=off */
	GPB7_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 4 1=off */
	GPB8_GPIO | GPIO_OUT | GPIO_VAL(1),

	/* camera interface (ignore it) */
	GPJ0_GPIO | GPIO_IN,
	GPJ1_GPIO | GPIO_IN,
	GPJ2_GPIO | GPIO_IN,
	GPJ3_GPIO | GPIO_IN,
	GPJ4_GPIO | GPIO_IN,
	GPJ5_GPIO | GPIO_IN,
	GPJ6_GPIO | GPIO_IN,
	GPJ7_GPIO | GPIO_IN,
	GPJ8_GPIO | GPIO_IN,
	GPJ9_GPIO | GPIO_IN,
	GPJ10_GPIO | GPIO_IN,
	GPJ11_GPIO | GPIO_IN,
	GPJ12_GPIO | GPIO_IN,

	/* I2C bus */
	GPE14_IICSCL,	/* external pull up */
	GPE15_IICSDA,	/* external pull up */

	GPA12_NGCS1,	/* CON5, pin 7 */
	GPA13_NGCS2,	/* CON5, pin 8 */
	GPA14_NGCS3,	/* CON5, pin 9 */
	GPA16_NGCS5,	/* CON5, pin 10 */

	/* UART2 (spare) */
	GPH4_TXD1,
	GPH5_RXD1,

	/* UART3 (spare) */
	GPH6_TXD2,
	GPH7_RXD2,
};

static int mini2440_mem_init(void)
{
	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c24xx_get_memory_size());

	return 0;
}
mem_initcall(mini2440_mem_init);

static int mini2440_devices_init(void)
{
	uint32_t reg;
	int i;

	/* ----------- configure the access to the outer space ---------- */
	for (i = 0; i < ARRAY_SIZE(pin_usage); i++)
		s3c_gpio_mode(pin_usage[i]);

	reg = readl(S3C_BWSCON);

	/* CS#4 to access the network controller */
	reg &= ~0x000f0000;
	reg |=  0x000d0000;	/* 16 bit */
	writel(0x1f4c, S3C_BANKCON4);

	writel(reg, S3C_BWSCON);

	/* release the reset signal to external devices */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	s3c24xx_add_nand(&nand_info);

	add_dm9000_device(0, S3C_CS4_BASE + 0x300, S3C_CS4_BASE + 0x304,
			  IORESOURCE_MEM_16BIT, &dm9000_data);
#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_del_partition("self_raw");
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_del_partition("env_raw");
	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	s3c24x0_bbu_nand_register_handler();
#endif
	s3c24xx_add_mci(&mci_data);
	s3c24xx_add_fb(&s3c24x0_fb_data);
	s3c24xx_add_ohci();
	armlinux_set_architecture(MACH_TYPE_MINI2440);

	return 0;
}

device_initcall(mini2440_devices_init);

static int mini2440_console_init(void)
{
	/*
	 * configure the UART1 right now, as barebox will
	 * start to send data immediately
	 */
	s3c_gpio_mode(GPH0_NCTS0);
	s3c_gpio_mode(GPH1_NRTS0);
	s3c_gpio_mode(GPH2_TXD0);
	s3c_gpio_mode(GPH3_RXD0);

	barebox_set_model("Friendlyarm mini2440");
	barebox_set_hostname("mini2440");

	s3c24xx_add_uart1();
	return 0;
}

console_initcall(mini2440_console_init);
