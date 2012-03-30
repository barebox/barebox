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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

static struct at91_ether_platform_data ether_pdata = {
	.flags = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

static int at91rm9200ek_mem_init(void)
{
	at91_add_device_sdram(64 * 1024 * 1024);

	return 0;
}
mem_initcall(at91rm9200ek_mem_init);

static struct at91_usbh_data ek_usbh_data = {
	.ports		= 2,
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

static int at91rm9200ek_devices_init(void)
{
	/*
	 * Correct IRDA resistor problem
	 * Set PA23_TXD in Output
	 */
	at91_set_gpio_output(AT91_PIN_PA23, 1);

	at91_add_device_eth(&ether_pdata);

	add_cfi_flash_device(0, AT91_CHIPSELECT_0, 0, 0);
	/* USB Host */
	at91_add_device_usbh_ohci(&ek_usbh_data);
	ek_device_add_leds();

#if defined(CONFIG_DRIVER_CFI) || defined(CONFIG_DRIVER_CFI_OLD)
	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");
#endif

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(MACH_TYPE_AT91RM9200EK);

	return 0;
}
device_initcall(at91rm9200ek_devices_init);

static int at91rm9200ek_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}
console_initcall(at91rm9200ek_console_init);
