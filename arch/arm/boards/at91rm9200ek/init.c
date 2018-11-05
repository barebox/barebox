/*
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
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
#include <gpio.h>
#include <fcntl.h>
#include <io.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <mach/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <spi/spi.h>

static struct macb_platform_data ether_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static int at91rm9200ek_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(at91rm9200ek_mem_init);

static struct at91_usbh_data ek_usbh_data = {
	.ports		= 2,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

#ifdef CONFIG_LED_GPIO
struct gpio_led ek_leds[] = {
	{
		.gpio		= AT91_PIN_PB0,
		.active_low	= 1,
		.led = {
			.name = "green",
		},
	}, {
		.gpio		= AT91_PIN_PB1,
		.active_low	= 1,
		.led = {
			.name = "yellow",
		},
	}, {
		.gpio		= AT91_PIN_PB2,
		.active_low	= 1,
		.led = {
			.name = "red",
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
	.vbus_pin	= AT91_PIN_PD4,
	.pullup_pin	= AT91_PIN_PD5,
};

static void ek_add_device_udc(void)
{
	at91_add_device_udc(&ek_udc_data);
	at91_set_multi_drive(ek_udc_data.pullup_pin, 1);	/* pullup_pin is connected to reset */
}
#else
static void ek_add_device_udc(void) {}
#endif

static struct spi_board_info ek_dataflash_spi_devices[] = {
	{	/* DataFlash chip */
		.name		= "mtd_dataflash",
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
	},
#ifdef CONFIG_MTD_AT91_DATAFLASH_CARD
	{	/* DataFlash card */
		.name		= "mtd_dataflash",
		.chip_select	= 1,
		.max_speed_hz	= 15 * 1000 * 1000,
	},
#endif
};

static struct spi_board_info ek_mmc_spi_devices[] = {
#if !defined(CONFIG_MTD_DATAFLASH)
	{
		.name		= "spi_mci",
		.chip_select	= 0,
		.max_speed_hz	= 20 * 1000 * 1000,
	},
#endif
#if !defined(CONFIG_MTD_AT91_DATAFLASH_CARD)
	{
		.name		= "spi_mci",
		.chip_select	= 1,
		.max_speed_hz	= 20 * 1000 * 1000,
	},
#endif
};

static unsigned spi0_standard_cs[] = { AT91_PIN_PA3, AT91_PIN_PA6 };
static struct at91_spi_platform_data spi_pdata = {
	.chipselect = spi0_standard_cs,
	.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
};

static void ek_add_device_spi(void)
{
	/* select mci0 as spi */
	at91_set_gpio_output(AT91_PIN_PB22, 0);
	spi_register_board_info(ek_dataflash_spi_devices, ARRAY_SIZE(ek_dataflash_spi_devices));
	spi_register_board_info(ek_mmc_spi_devices, ARRAY_SIZE(ek_mmc_spi_devices));
	at91_add_device_spi(0, &spi_pdata);
}

static int at91rm9200ek_devices_init(void)
{
	/*
	 * Correct IRDA resistor problem
	 * Set PA23_TXD in Output
	 */
	at91_set_gpio_output(AT91_PIN_PA23, 1);

	at91_add_device_eth(0, &ether_pdata);

	add_cfi_flash_device(0, AT91_CHIPSELECT_0, SZ_8M, 0);
	/* USB Host */
	at91_add_device_usbh_ohci(&ek_usbh_data);
	ek_device_add_leds();
	ek_add_device_udc();
	ek_add_device_spi();

#if defined(CONFIG_DRIVER_CFI) || defined(CONFIG_DRIVER_CFI_OLD)
	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");
#endif

	armlinux_set_architecture(MACH_TYPE_AT91RM9200EK);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_at91rm9200ek);

	return 0;
}
device_initcall(at91rm9200ek_devices_init);

static int at91rm9200ek_console_init(void)
{
	barebox_set_model("Atmel at91rm9200-ek");
	barebox_set_hostname("at91rm9200-ek");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91rm9200ek_console_init);

static int at91rm9200ek_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(at91rm9200ek_main_clock);
