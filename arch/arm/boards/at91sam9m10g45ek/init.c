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
#include <mci.h>
#include <init.h>
#include <gpio.h>
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
#include <mach/io.h>
#include <mach/at91sam9_smc.h>
#include <gpio_keys.h>
#include <readkey.h>
#include <spi/spi.h>
#include <input/input.h>

/*
 * board revision encoding
 * bit 0-3: lcd type
 *	0 => truly TFT1N4633-E (sam9m10g45-ek)
 *	1 => LG philips LB043WQ1 (sam9m10-ekes and sam9g45-ekes)
 */
#define HAVE_LCD_TRULY_TFT1N4633E	(0 << 0)
#define HAVE_LCD_LG_LB043WQ1		(1 << 0)
static void ek_set_board_revision(void)
{
	u32 rev;

#ifdef CONFIG_LCD_LG_LB043WQ1
		rev = HAVE_LCD_LG_LB043WQ1;
#else
		rev = HAVE_LCD_TRULY_TFT1N4633E;
#endif
		armlinux_set_revision(rev);
}

static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC8,
	.enable_pin	= AT91_PIN_PC14,
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
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 4,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 4,
	.nwe_pulse		= 4,

	.read_cycle		= 7,
	.write_cycle		= 7,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 3,
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
static struct atmel_mci_platform_data ek_mci_data = {
	.bus_width	= 4,
	.detect_pin	= AT91_PIN_PD10,
	.wp_pin		= -EINVAL,
};

static void ek_add_device_mci(void)
{
	at91_add_device_mci(0, &ek_mci_data);
}
#else
static void ek_add_device_mci(void) {}
#endif

static const struct spi_board_info ek_spi_devices[] = {
	{
		.name		= "mtd_dataflash",
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	}
};

static void ek_add_device_spi(void)
{
	spi_register_board_info(ek_spi_devices,
			ARRAY_SIZE(ek_spi_devices));
	at91_add_device_spi(0, NULL);
}

#ifdef CONFIG_LED_GPIO
struct gpio_led ek_leds[] = {
	{
		.gpio		= AT91_PIN_PD30,
		.led = {
			.name = "d8",
		},
	}, {
		.active_low	= 1,
		.gpio		= AT91_PIN_PD0,
		.led = {
			.name = "d6",
		},
	}, {
		.active_low	= 1,
		.gpio		= AT91_PIN_PD31,
		.led = {
			.name = "d7",
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
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &ek_leds[0].led);
}
#else
static void ek_device_add_leds(void) {}
#endif

#ifdef CONFIG_KEYBOARD_GPIO
struct gpio_keys_button keys[] = {
	{
		.code = KEY_HOME,
		.gpio = AT91_PIN_PB6,
	}, {
		.code = KEY_ENTER,
		.gpio = AT91_PIN_PB7,
	}, {
		.code = KEY_LEFT,
		.gpio = AT91_PIN_PB14,
	}, {
		.code = KEY_RIGHT,
		.gpio = AT91_PIN_PB15,
	}, {
		.code = KEY_UP,
		.gpio = AT91_PIN_PB16,
	}, {
		.code = KEY_DOWN,
		.gpio = AT91_PIN_PB17,
	}, {
		.code = KEY_ENTER,
		.gpio = AT91_PIN_PB18,
	},
};

struct gpio_keys_platform_data gk_pdata = {
	.buttons = keys,
	.nbuttons = ARRAY_SIZE(keys),
};

static void ek_device_add_keyboard(void)
{
	int i;

	for (i = 0; i < gk_pdata.nbuttons; i++) {
		/* user push button, pull up enabled */
		keys[i].active_low = 1;
		at91_set_GPIO_periph(keys[i].gpio, keys[i].active_low);
		at91_set_deglitch(keys[i].gpio, 1);
	}

	add_gpio_keys_device(DEVICE_ID_DYNAMIC, &gk_pdata);
}
#else
static void ek_device_add_keyboard(void) {}
#endif

#if defined(CONFIG_USB_OHCI) || defined(CONFIG_USB_EHCI)
/*
 * USB HS Host port (common to OHCI & EHCI)
 */
static struct at91_usbh_data ek_usbh_hs_data = {
	.ports			= 2,
	.vbus_pin		= {AT91_PIN_PD1, AT91_PIN_PD3},
	.vbus_pin_active_low	= {1, 1},
};

static void ek_add_device_usb(void)
{
	at91_add_device_usbh_ohci(&ek_usbh_hs_data);
	at91_add_device_usbh_ehci(&ek_usbh_hs_data);
}
#else
static void ek_add_device_usb(void) {}
#endif

static int at91sam9m10g45ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91sam9m10g45ek_mem_init);

#if defined(CONFIG_DRIVER_VIDEO_ATMEL)
static struct fb_videomode at91_tft_vga_modes[] = {
	{
		.name		= "LG",
		.refresh	= 60,
		.xres		= 480,		.yres		= 272,
		.pixclock	= KHZ2PICOS(9000),

		.left_margin	= 1,		.right_margin	= 1,
		.upper_margin	= 40,		.lower_margin	= 1,
		.hsync_len	= 45,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
};

#define AT91SAM9G45_DEFAULT_LCDCON2	(ATMEL_LCDC_MEMOR_LITTLE \
					| ATMEL_LCDC_DISTYPE_TFT \
					| ATMEL_LCDC_CLKMOD_ALWAYSACTIVE)

/* Driver datas */
static struct atmel_lcdfb_platform_data ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.default_bpp			= 32,
	.default_dmacon			= ATMEL_LCDC_DMAEN,
	.default_lcdcon2		= AT91SAM9G45_DEFAULT_LCDCON2,
	.guard_time			= 9,
	.lcd_wiring_mode		= ATMEL_LCDC_WIRING_RGB,
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


static int at91sam9m10g45ek_devices_init(void)
{
	ek_add_device_nand();
	ek_add_device_spi();
	at91_add_device_eth(0, &macb_pdata);
	ek_add_device_mci();
	ek_add_device_usb();
	ek_device_add_leds();
	ek_device_add_keyboard();
	ek_add_device_lcdc();

	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	armlinux_set_architecture(MACH_TYPE_AT91SAM9M10G45EK);
	ek_set_board_revision();

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_at91sam9m10g45ek);

	return 0;
}
device_initcall(at91sam9m10g45ek_devices_init);

static int at91sam9m10g45ek_console_init(void)
{
	barebox_set_model("Atmel at91sam9m10g45-ek");
	barebox_set_hostname("at91sam9m10g45-ek");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91sam9m10g45ek_console_init);

static int at91sam9m10g45ek_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(at91sam9m10g45ek_main_clock);
