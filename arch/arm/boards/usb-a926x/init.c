/*
 * Copyright (C) 2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <envfs.h>
#include <mach/hardware.h>
#include <mach/at91sam926x.h>
#include <nand.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <linux/clk.h>
#include <mach/board.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91sam9_sdramc.h>
#include <gpio.h>
#include <led.h>
#include <mach/iomux.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <gpio_keys.h>
#include <readkey.h>
#include <spi/spi.h>
#include <input/input.h>

static void usb_a9260_set_board_type(void)
{
	if (machine_is_usb_a9g20())
		armlinux_set_architecture(MACH_TYPE_USB_A9G20);
	else if (machine_is_usb_a9263())
		armlinux_set_architecture(MACH_TYPE_USB_A9263);
	else
		armlinux_set_architecture(MACH_TYPE_USB_A9260);
}

#if defined(CONFIG_NAND_ATMEL)
static struct atmel_nand_data nand_pdata = {
	.ale		= 21,
	.cle		= 22,
	.det_pin	= -EINVAL,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.ecc_mode	= NAND_ECC_SOFT,
	.on_flash_bbt	= 1,
};

static struct sam9_smc_config usb_a9260_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 2,
};

static struct sam9_smc_config usb_a9g20_nand_smc_config = {
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

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_DBW_8,
	.tdf_cycles		= 3,
};

static void usb_a9260_add_device_nand(void)
{
	/* configure chip-select 3 (NAND) */
	if (machine_is_usb_a9g20())
		sam9_smc_configure(0, 3, &usb_a9g20_nand_smc_config);
	else
		sam9_smc_configure(0, 3, &usb_a9260_nand_smc_config);

	if (machine_is_usb_a9263()) {
		nand_pdata.rdy_pin	= AT91_PIN_PA22;
		nand_pdata.enable_pin	= AT91_PIN_PD15;
	}

	at91_add_device_nand(&nand_pdata);
}
#else
static void usb_a9260_add_device_nand(void) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
static struct macb_platform_data macb_pdata = {
	.phy_interface	= PHY_INTERFACE_MODE_RMII,
	.phy_addr	= -1,
};

static void usb_a9260_phy_reset(void)
{
	at91_set_gpio_input(AT91_PIN_PA14, 0);
	at91_set_gpio_input(AT91_PIN_PA15, 0);
	at91_set_gpio_input(AT91_PIN_PA17, 0);
	at91_set_gpio_input(AT91_PIN_PA25, 0);
	at91_set_gpio_input(AT91_PIN_PA26, 0);
	at91_set_gpio_input(AT91_PIN_PA28, 0);

	/* same address for the different supported SoCs */
	at91sam_phy_reset(IOMEM(AT91SAM926X_BASE_RSTC));
}

static void usb_a9260_add_device_eth(void)
{
	usb_a9260_phy_reset();
	at91_add_device_eth(0, &macb_pdata);
}
#else
static void usb_a9260_add_device_eth(void) {}
#endif

#if defined(CONFIG_DRIVER_SPI_ATMEL)
static const struct spi_board_info usb_a9263_spi_devices[] = {
	{
		.name		= "mtd_dataflash",
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	}
};

static const struct spi_board_info usb_a9g20_spi_devices[] = {
	{
		.name		= "spi_mci",
		.chip_select	= 0,
		.max_speed_hz	= 25 * 1000 * 1000,
		.bus_num	= 1,
	}
};

static unsigned spi0_standard_cs_a9263[] = { AT91_PIN_PA5 };
static struct at91_spi_platform_data spi_a9263_pdata = {
	.chipselect = spi0_standard_cs_a9263,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs_a9263),
};

static unsigned spi0_standard_cs_a9g20[] = { AT91_PIN_PB3 };
static struct at91_spi_platform_data spi_a9g20_pdata = {
	.chipselect = spi0_standard_cs_a9g20,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs_a9g20),
};

static void usb_a9260_add_spi(void)
{
	if (machine_is_usb_a9263()) {
		spi_register_board_info(usb_a9263_spi_devices,
				ARRAY_SIZE(usb_a9263_spi_devices));
		at91_add_device_spi(0, &spi_a9263_pdata);
	} else if (machine_is_usb_a9g20() && at91sam9260_is_low_power_sdram()) {
		spi_register_board_info(usb_a9g20_spi_devices,
				ARRAY_SIZE(usb_a9g20_spi_devices));
		at91_add_device_spi(1, &spi_a9g20_pdata);
	}
}
#else
static void usb_a9260_add_spi(void) {}
#endif

#if defined(CONFIG_MCI_ATMEL)
static struct atmel_mci_platform_data __initdata usb_a9260_mci_data = {
	.bus_width	= 4,
	.detect_pin     = -EINVAL,
	.wp_pin		= -EINVAL,
};

static void usb_a9260_add_device_mci(void)
{
	at91_add_device_mci(0, &usb_a9260_mci_data);
}
#else
static void usb_a9260_add_device_mci(void) {}
#endif

#if defined(CONFIG_USB_OHCI)
static struct at91_usbh_data ek_usbh_data = {
	.ports		= 2,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

static void usb_a9260_add_device_usb(void)
{
	at91_add_device_usbh_ohci(&ek_usbh_data);
}
#else
static void usb_a9260_add_device_usb(void) {}
#endif

#ifdef CONFIG_USB_GADGET_DRIVER_AT91
/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PB11,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

static void __init ek_add_device_udc(void)
{
	if (machine_is_usb_a9260() || machine_is_usb_a9g20())
		ek_udc_data.vbus_pin = AT91_PIN_PC5;

	at91_add_device_udc(&ek_udc_data);
}
#else
static void __init ek_add_device_udc(void) {}
#endif

#ifdef CONFIG_LED_GPIO
struct gpio_led led = {
	.gpio = AT91_PIN_PB21,
	.led = {
		.name = "user_led",
	},
};

static void __init ek_add_led(void)
{
	if (machine_is_usb_a9263())
		led.active_low = 1;

	at91_set_gpio_output(led.gpio, led.active_low);
	led_gpio_register(&led);
}
#else
static void ek_add_led(void) {}
#endif

static int usb_a9260_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(usb_a9260_mem_init);

static void __init ek_add_device_button(void)
{
	at91_set_GPIO_periph(AT91_PIN_PB10, 1);	/* user push button, pull up enabled */
	at91_set_deglitch(AT91_PIN_PB10, 1);

	export_env_ull("dfu_button", AT91_PIN_PB10);
}

#ifdef CONFIG_CALAO_DAB_MMX
struct gpio_led dab_mmx_leds[] = {
	{
		.gpio = AT91_PIN_PB20,
		.led = {
			.name = "user_led1",
		},
	}, {
		.gpio = AT91_PIN_PB21,
		.led = {
			.name = "user_led2",
		},
	}, {
		.gpio = AT91_PIN_PB22,
		.led = {
			.name = "user_led3",
		},
	}, {
		.gpio = AT91_PIN_PB23,
		.led = {
			.name = "user_led4",
		},
	}, {
		.gpio = AT91_PIN_PB24,
		.led = {
			.name = "red",
		},
	}, {
		.gpio = AT91_PIN_PB30,
		.led = {
			.name = "orange",
		},
	}, {
		.gpio = AT91_PIN_PB31,
		.led = {
			.name = "green",
		},
	},
};

#ifdef CONFIG_KEYBOARD_GPIO
struct gpio_keys_button keys[] = {
	{
		.code = KEY_UP,
		.gpio = AT91_PIN_PB25,
	}, {
		.code = KEY_HOME,
		.gpio = AT91_PIN_PB13,
	}, {
		.code = KEY_DOWN,
		.gpio = AT91_PIN_PA26,
	}, {
		.code = KEY_ENTER,
		.gpio = AT91_PIN_PC9,
	},
};

struct gpio_keys_platform_data gk_pdata = {
	.buttons = keys,
	.nbuttons = ARRAY_SIZE(keys),
};

static void usb_a9260_keyboard_device_dab_mmx(void)
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
static void usb_a9260_keyboard_device_dab_mmx(void) {}
#endif

static void usb_a9260_device_dab_mmx(void)
{
	int i;

	usb_a9260_keyboard_device_dab_mmx();

	for (i = 0; i < ARRAY_SIZE(dab_mmx_leds); i++) {
		dab_mmx_leds[i].active_low = 1;
		at91_set_gpio_output(dab_mmx_leds[i].gpio, dab_mmx_leds[i].active_low);
		led_gpio_register(&dab_mmx_leds[i]);
	}
}
#else
static void usb_a9260_device_dab_mmx(void) {}
#endif

static int usb_a9260_devices_init(void)
{
	usb_a9260_add_device_nand();
	usb_a9260_add_device_mci();
	usb_a9260_add_device_eth();
	usb_a9260_add_spi();
	usb_a9260_add_device_usb();
	ek_add_device_udc();
	ek_add_led();
	ek_add_device_button();
	usb_a9260_device_dab_mmx();

	usb_a9260_set_board_type();

	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "at91bootstrap_raw");
	dev_add_bb_dev("at91bootstrap_raw", "at91bootstrap");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_256K + SZ_128K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
	devfs_add_partition("nand0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw1");
	dev_add_bb_dev("env_raw1", "env1");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_usb_a926x);

	return 0;
}
device_initcall(usb_a9260_devices_init);

#ifndef CONFIG_CONSOLE_NONE
static int usb_a9260_console_init(void)
{
	struct device_d *dev;

	if (machine_is_usb_a9260()) {
		barebox_set_model("Calao USB-A9260");
		barebox_set_hostname("usb-a9260");
	} else if (machine_is_usb_a9g20()) {
		barebox_set_model("Calao USB-A9G20");
		barebox_set_hostname("usb-a9g20");
	} else {
		barebox_set_model("Calao USB-A9263");
		barebox_set_hostname("usb-a9263");
	}

	at91_register_uart(0, 0);

	if (IS_ENABLED(CONFIG_CALAO_DAB_MMX)) {
		at91_register_uart(2, 0);

		dev = at91_register_uart(4, 0);
		dev_set_param(dev, "active", "");
	}

	return 0;
}
console_initcall(usb_a9260_console_init);
#endif

static int usb_a9260_main_clock(void)
{
	at91_set_main_clock(12000000);
	return 0;
}
pure_initcall(usb_a9260_main_clock);
