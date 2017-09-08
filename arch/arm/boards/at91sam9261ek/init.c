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
#include <gpio.h>
#include <mach/io.h>
#include <mach/iomux.h>
#include <mach/at91sam9_smc.h>
#include <platform_data/eth-dm9000.h>
#include <gpio_keys.h>
#include <readkey.h>
#include <led.h>
#include <spi/spi.h>
#include <input/input.h>

static struct atmel_nand_data nand_pdata = {
	.ale		= 22,
	.cle		= 21,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC15,
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

/*
 * DM9000 ethernet device
 */
#if defined(CONFIG_DRIVER_NET_DM9K)
static struct dm9000_platform_data dm9000_data = {
	.srom		= 0,
};

/*
 * SMC timings for the DM9000.
 * Note: These timings were calculated for MASTER_CLOCK = 100000000 according to the DM9000 timings.
 */
static struct sam9_smc_config __initdata dm9000_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 8,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 8,
	.nwe_pulse		= 4,

	.read_cycle		= 16,
	.write_cycle		= 16,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_BAT_WRITE | AT91_SMC_DBW_16,
	.tdf_cycles		= 1,
};

static void __init ek_add_device_dm9000(void)
{
	/* Configure chip-select 2 (DM9000) */
	sam9_smc_configure(0, 2, &dm9000_smc_config);

	/* Configure Reset signal as output */
	at91_set_gpio_output(AT91_PIN_PC10, 0);

	/* Configure Interrupt pin as input, no pull-up */
	at91_set_gpio_input(AT91_PIN_PC11, 0);

	add_dm9000_device(0, AT91_CHIPSELECT_2, AT91_CHIPSELECT_2 + 4,
			  IORESOURCE_MEM_16BIT, &dm9000_data);
}
#else
static void __init ek_add_device_dm9000(void) {}
#endif /* CONFIG_DRIVER_NET_DM9K */

#if defined(CONFIG_USB_GADGET_DRIVER_AT91)
/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PB29,
	.pullup_pin	= 0,
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
/* TFT */
static struct fb_videomode at91_tft_vga_modes[] = {
	{
	        .name           = "TX09D50VM1CCA @ 60",
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

#define AT91SAM9261_DEFAULT_TFT_LCDCON2	(ATMEL_LCDC_MEMOR_LITTLE \
					| ATMEL_LCDC_DISTYPE_TFT    \
					| ATMEL_LCDC_CLKMOD_ALWAYSACTIVE)

static struct atmel_lcdfb_platform_data ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.default_bpp			= 16,
	.default_dmacon			= ATMEL_LCDC_DMAEN,
	.default_lcdcon2		= AT91SAM9261_DEFAULT_TFT_LCDCON2,
	.guard_time			= 1,
	.gpio_power_control		= AT91_PIN_PA12,
	.gpio_power_control_active_low	= true,
	.mode_list			= at91_tft_vga_modes,
	.num_modes			= ARRAY_SIZE(at91_tft_vga_modes),
};

static void ek_add_device_lcdc(void)
{
	if (machine_is_at91sam9g10ek())
		ek_lcdc_data.lcd_wiring_mode = ATMEL_LCDC_WIRING_RGB;

	at91_add_device_lcdc(&ek_lcdc_data);
}

#else
static void ek_add_device_lcdc(void) {}
#endif

#ifdef CONFIG_KEYBOARD_GPIO
struct gpio_keys_button keys[] = {
	{
		.code = KEY_UP,
		.gpio = AT91_PIN_PA26,
	}, {
		.code = KEY_DOWN,
		.gpio = AT91_PIN_PA25,
	}, {
		.code = KEY_ENTER,
		.gpio = AT91_PIN_PA24,
	},
};

struct gpio_keys_platform_data gk_pdata = {
	.buttons = keys,
	.nbuttons = ARRAY_SIZE(keys),
};

static void ek_add_device_keyboard_buttons(void)
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
static void ek_add_device_keyboard_buttons(void) {}
#endif

static void __init ek_add_device_buttons(void)
{
	at91_set_gpio_input(AT91_PIN_PA27, 1);
	at91_set_deglitch(AT91_PIN_PA27, 1);
	export_env_ull("dfu_button", AT91_PIN_PA27);
	ek_add_device_keyboard_buttons();
}

#ifdef CONFIG_LED_GPIO
struct gpio_led ek_leds[] = {
	{
		.gpio		= AT91_PIN_PA23,
		.led = {
			.name = "ds1",
		},
	}, {
		.gpio		= AT91_PIN_PA14,
		.active_low	= 1,
		.led = {
			.name = "ds7",
		},
	}, {
		.gpio		= AT91_PIN_PA13,
		.active_low	= 1,
		.led = {
			.name = "ds8",
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

/*
 * SPI related devices
 */
#if defined(CONFIG_DRIVER_SPI_ATMEL)
/*
 * SPI devices
 */
static struct spi_board_info ek_spi_devices[] = {
	{	/* DataFlash chip */
		.name		= "mtd_dataflash",
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	},
#if defined(CONFIG_MTD_AT91_DATAFLASH_CARD)
	{	/* DataFlash card - jumper (J12) configurable to CS3 or CS0 */
		.name		= "mtd_dataflash",
		.chip_select	= 1,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	},
#endif
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PA3, AT91_PIN_PA6};
static struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void ek_add_device_spi(void)
{
	spi_register_board_info(ek_spi_devices,
				ARRAY_SIZE(ek_spi_devices));
	at91_add_device_spi(0, &spi_pdata);
}
#else
static void ek_add_device_spi(void) {}
#endif

static int at91sam9261ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91sam9261ek_mem_init);

static int at91sam9261ek_devices_init(void)
{
	u32 barebox_part_start;
	u32 barebox_part_size;

	ek_add_device_nand();
	ek_add_device_dm9000();
	ek_add_device_udc();
	ek_add_device_buttons();
	ek_device_add_leds();
	ek_add_device_lcdc();
	ek_add_device_spi();

	if (IS_ENABLED(CONFIG_AT91_LOAD_BAREBOX_SRAM)) {
		barebox_part_start = 0;
		barebox_part_size = SZ_256K + SZ_128K;
		export_env_ull("borebox_first_stage", 1);
	} else {
		devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
		barebox_part_start = SZ_128K;
		barebox_part_size = SZ_256K;
	}
	devfs_add_partition("nand0", barebox_part_start, barebox_part_size,
			    DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	if (machine_is_at91sam9g10ek())
		armlinux_set_architecture(MACH_TYPE_AT91SAM9G10EK);
	else
		armlinux_set_architecture(MACH_TYPE_AT91SAM9261EK);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_at91sam9261ek);

	return 0;
}

device_initcall(at91sam9261ek_devices_init);

static int at91sam9261ek_console_init(void)
{
	if (machine_is_at91sam9g10ek()) {
		barebox_set_model("Atmel at91sam9g10-ek");
		barebox_set_hostname("at91sam9g10-ek");
	} else {
		barebox_set_model("Atmel at91sam9261-ek");
		barebox_set_hostname("at91sam9261-ek");
	}

	at91_register_uart(0, 0);
	return 0;
}

console_initcall(at91sam9261ek_console_init);

static int at91sam9261ek_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(at91sam9261ek_main_clock);
