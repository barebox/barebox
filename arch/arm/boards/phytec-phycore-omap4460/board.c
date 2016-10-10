/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
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
#include <console.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <io.h>
#include <envfs.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-devices.h>
#include <mach/omap4-clock.h>
#include <mach/omap-fb.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/control.h>
#include <linux/err.h>
#include <linux/sizes.h>
#include <partition.h>
#include <nand.h>
#include <asm/mmu.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>
#include <i2c/i2c.h>

static int pcm049_console_init(void)
{
	barebox_set_model("Phytec phyCORE-OMAP4460");
	barebox_set_hostname("phycore-omap4460");

	omap44xx_add_uart3();

	return 0;
}
console_initcall(pcm049_console_init);

static int pcm049_mem_init(void)
{
#ifdef CONFIG_1024MB_DDR2RAM
	omap_add_ram0(SZ_1G);
#else
	omap_add_ram0(SZ_512M);
#endif

	omap44xx_add_sram0();
	return 0;
}
mem_initcall(pcm049_mem_init);

static struct gpmc_config net_cfg = {
	.cfg = {
		0xc1001000,	/* CONF1 */
		0x00070700,	/* CONF2 */
		0x00000000,	/* CONF3 */
		0x07000700,	/* CONF4 */
		0x09060909,	/* CONF5 */
		0x000003c2,	/* CONF6 */
	},
	.base = 0x2C000000,
	.size = GPMC_SIZE_16M,
};

static void pcm049_network_init(void)
{
	gpmc_cs_config(5, &net_cfg);

	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL, 0x2C000000, 0x4000,
			   IORESOURCE_MEM, NULL);
}

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("twl6030", 0x48),
	},
};

static struct gpmc_nand_platform_data nand_plat = {
	.wait_mon_pin = 1,
	.ecc_mode = OMAP_ECC_BCH8_CODE_HW,
	.nand_cfg = &omap4_nand_cfg,
};

static struct omapfb_display const pcm049_displays[] = {
	{
		.mode	= {
			.name		= "pd050vl1",
			.refresh	= 60,
			.xres		= 640,
			.yres		= 480,
			.pixclock	= 25000,
			.left_margin	= 46,
			.right_margin	= 18,
			.hsync_len	= 96,
			.upper_margin	= 33,
			.lower_margin	= 10,
			.vsync_len	= 2,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* Prime-View PM070WL4 */
	{
		.mode	= {
			.name		= "pm070wl4",
			.refresh	= 60,
			.xres		= 800,
			.yres		= 480,
			.pixclock	= 32000,
			.left_margin	= 86,
			.right_margin	= 42,
			.hsync_len	= 128,
			.lower_margin	= 10,
			.upper_margin	= 33,
			.vsync_len	= 2,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* Prime-View PD104SLF */
	{
		.mode	= {
			.name		= "pd104slf",
			.refresh	= 60,
			.xres		= 800,
			.yres		= 600,
			.pixclock	= 40000,
			.left_margin	= 86,
			.right_margin	= 42,
			.hsync_len	= 128,
			.lower_margin	= 1,
			.upper_margin	= 23,
			.vsync_len	= 4,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* EDT ETM0350G0DH6 */
	{
		.mode	= {
			.name		= "edt_etm0350G0dh6",
			.refresh	= 60,
			.xres		= 320,
			.yres		= 240,
			.pixclock	= 15720,
			.left_margin	= 68,
			.right_margin	= 20,
			.hsync_len	= 88,
			.lower_margin	= 4,
			.upper_margin	= 18,
			.vsync_len	= 22,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* EDT ETM0430G0DH6 */
	{
		.mode	= {
			.name		= "edt_etm0430G0dh6",
			.refresh	= 60,
			.xres		= 480,
			.yres		= 272,
			.pixclock	= 9000,
			.left_margin	= 2,
			.right_margin	= 2,
			.hsync_len	= 41,
			.lower_margin	= 2,
			.upper_margin	= 2,
			.vsync_len	= 10,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* EDT ETMV570G2DHU */
	{
		.mode	= {
			.name		= "edt_etmv570G2dhu",
			.refresh	= 60,
			.xres		= 640,
			.yres		= 480,
			.pixclock	= 25175,
			.left_margin	= 114,
			.right_margin	= 16,
			.hsync_len	= 30,
			.lower_margin	= 10,
			.upper_margin	= 35,
			.vsync_len	= 3,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},
	/* ETD ETM0700G0DH6 */
	{
		.mode	= {
			.name		= "edt_etm0700G0dh6",
			.refresh	= 60,
			.xres		= 800,
			.yres		= 480,
			.pixclock	= 33260,
			.left_margin	= 216,
			.right_margin	= 40,
			.hsync_len	= 128,
			.lower_margin	= 10,
			.upper_margin	= 35,
			.vsync_len	= 2,
		},

		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),
	},

	/* CHIMEI G104X1-L03 */
	{
		.mode = {
			.name		= "g104x1",
			.refresh	= 60,
			.xres		= 1024,
			.yres		= 768,
			.pixclock	= 64000,
			.left_margin	= 320,
			.right_margin	= 1,
			.hsync_len	= 320,
			.upper_margin	= 38,
			.lower_margin	= 38,
			.vsync_len	= 2,
		},
		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
				OMAP_DSS_LCD_DATALINES_24),

		.power_on_delay		= 50,
		.power_off_delay	= 100,
	},
};

#define GPIO_DISPENABLE			  118
#define GPIO_BACKLIGHT			  122

static void pcm049_fb_enable(int e)
{
	gpio_direction_output(GPIO_DISPENABLE, e);
	gpio_direction_output(GPIO_BACKLIGHT, e);
}

static struct omapfb_platform_data pcm049_fb_data = {
	.displays	= pcm049_displays,
	.num_displays	= ARRAY_SIZE(pcm049_displays),

	.dss_clk_hz	= 19200000,

	.bpp		= 32,
	.enable		= pcm049_fb_enable,
};

static int pcm049_devices_init(void)
{
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	omap44xx_add_i2c1(NULL);
	omap44xx_add_mmc1(NULL);

	gpmc_generic_init(0x10);

	if (IS_ENABLED(CONFIG_DRIVER_NET_SMC911X))
		pcm049_network_init();

	omap_add_gpmc_nand_device(&nand_plat);

#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "xload_raw");
	dev_add_bb_dev("xload_raw", "xload");
	devfs_add_partition("nand0", SZ_128K, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_128K + SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif

	armlinux_set_architecture(MACH_TYPE_PCM049);

	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_OMAP))
		omap_add_display(&pcm049_fb_data);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_phytec_phycore_omap4460);

	return 0;
}
device_initcall(pcm049_devices_init);
