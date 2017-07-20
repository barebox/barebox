/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
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
#include <envfs.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <mach/hardware.h>
#include <nand.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <gpio.h>
#include <mach/io.h>
#include <mach/at91sam9_smc.h>

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PA22,
	.enable_pin	= AT91_PIN_PD15,
	.ecc_mode	= NAND_ECC_SOFT,
#if defined(CONFIG_MTD_NAND_ATMEL_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config ek_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 1,
	.ncs_write_setup	= 0,
	.nwe_setup		= 1,

	.ncs_read_pulse		= 3,
	.nrd_pulse		= 3,
	.ncs_write_pulse	= 3,
	.nwe_pulse		= 3,

	.read_cycle		= 5,
	.write_cycle		= 5,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 2,
};

static void ek_add_device_nand(void)
{
	/* setup bus-width (8 or 16) */
	if (nand_pdata.bus_width_16)
		ek_nand_smc_config.mode |= AT91_SMC_DBW_16;
	else
		ek_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

	at91_add_device_nand(&nand_pdata);
}

static struct macb_platform_data macb_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata ek_mci_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PE18,
	.wp_pin		= AT91_PIN_PE19,
};

static void ek_add_device_mci(void)
{
	at91_add_device_mci(1, &ek_mci_data);
}
#else
static void ek_add_device_mci(void) {}
#endif

#ifdef CONFIG_LED_GPIO
struct gpio_led ek_leds[] = {
	{
		.gpio		= AT91_PIN_PC29,
		.active_low	= 1,
		.led = {
			.name = "ds2",
		},
	}, {
		.gpio		= AT91_PIN_PB7,
		.led = {
			.name = "ds3",
		},
	},
};

static void ek_device_add_leds(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ek_leds); i++) {
		at91_set_gpio_output(ek_leds[i].gpio, ek_leds[i].active_low);
		led_gpio_register(&ek_leds[i]);
	}
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &ek_leds[1].led);
}
#else
static void ek_device_add_leds(void) {}
#endif

#if defined(CONFIG_USB_GADGET_DRIVER_AT91)
/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PA25,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

static void ek_add_device_udc(void)
{
	at91_add_device_udc(&ek_udc_data);
}
#else
static void ek_add_device_udc(void) {}
#endif

/*
 * LCD Controller
 */
#if defined(CONFIG_DRIVER_VIDEO_ATMEL)
static struct fb_videomode at91_tft_vga_modes[] = {
	{
		.name		= "TX09D50VM1CCA @ 60",
		.refresh	= 60,
		.xres		= 240,		.yres		= 320,
		.pixclock	= KHZ2PICOS(4965),

		.left_margin	= 1,		.right_margin	= 33,
		.upper_margin	= 1,		.lower_margin	= 0,
		.hsync_len	= 5,		.vsync_len	= 1,

		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
};

#define AT91SAM9263_DEFAULT_LCDCON2 	(ATMEL_LCDC_MEMOR_LITTLE \
					| ATMEL_LCDC_DISTYPE_TFT \
					| ATMEL_LCDC_CLKMOD_ALWAYSACTIVE)

/* Driver datas */
static struct atmel_lcdfb_platform_data ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.default_bpp			= 16,
	.default_dmacon			= ATMEL_LCDC_DMAEN,
	.default_lcdcon2		= AT91SAM9263_DEFAULT_LCDCON2,
	.guard_time			= 1,
	.gpio_power_control		= AT91_PIN_PA30,
	.mode_list			= at91_tft_vga_modes,
	.num_modes			= ARRAY_SIZE(at91_tft_vga_modes),
};

static void ek_add_device_lcdc(void)
{
	at91_add_device_lcdc(&ek_lcdc_data);
}

#else
static void ek_add_device_lcdc(void) {}
#endif

static void __init ek_add_device_buttons(void)
{
	at91_set_gpio_input(AT91_PIN_PC5, 1);
	at91_set_deglitch(AT91_PIN_PC5, 1);
	export_env_ull("dfu_button", AT91_PIN_PC5);
	at91_set_gpio_input(AT91_PIN_PC4, 1);
	at91_set_deglitch(AT91_PIN_PC4, 1);
	export_env_ull("right_click", AT91_PIN_PC4);
}

static int at91sam9263ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91sam9263ek_mem_init);

static int at91sam9263ek_devices_init(void)
{
	/*
	 * PB27 enables the 50MHz oscillator for Ethernet PHY
	 * 1 - enable
	 * 0 - disable
	 */
	at91_set_gpio_output(AT91_PIN_PB27, 1);
	gpio_set_value(AT91_PIN_PB27, 1); /* 1- enable, 0 - disable */

	ek_add_device_nand();
	at91_add_device_eth(0, &macb_pdata);
	add_cfi_flash_device(0, AT91_CHIPSELECT_0, 8 * 1024 * 1024, 0);
	ek_add_device_mci();
	ek_device_add_leds();
	ek_add_device_udc();
	ek_add_device_buttons();
	ek_add_device_lcdc();

	if (IS_ENABLED(CONFIG_DRIVER_CFI) && cdev_by_name("nor0")) {
		devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self");
		devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");
	} else if (IS_ENABLED(CONFIG_NAND_ATMEL)) {
		devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
		dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
		devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
		dev_add_bb_dev("env_raw1", "env1");
	}

	armlinux_set_architecture(MACH_TYPE_AT91SAM9263EK);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_at91sam9263ek);

	return 0;
}

device_initcall(at91sam9263ek_devices_init);

static int at91sam9263ek_console_init(void)
{
	barebox_set_model("Atmel at91sam9263-ek");
	barebox_set_hostname("at91sam9263-ek");

	at91_register_uart(0, 0);
	return 0;
}

console_initcall(at91sam9263ek_console_init);

static int at91sam9263ek_main_clock(void)
{
	at91_set_main_clock(16367660);
	return 0;
}
pure_initcall(at91sam9263ek_main_clock);
