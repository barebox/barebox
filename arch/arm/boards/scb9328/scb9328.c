/*
 * Copyright (C) 2004 Sascha Hauer, Synertronixx GmbH
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
#include <generated/mach-types.h>
#include <mach/imx-regs.h>
#include <asm/armlinux.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <dm9000.h>
#include <led.h>

static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",

	.map_base = 0x10000000,
	.size     = 16 * 1024 * 1024,
};

static struct memory_platform_data sdram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0x08000000,
	.size     = 16 * 1024 * 1024,
	.platform_data = &sdram_pdata,
};

static struct dm9000_platform_data dm9000_data = {
	.iobase   = 0x16000000,
	.iodata   = 0x16000004,
	.buswidth = DM9000_WIDTH_16,
	.srom     = 1,
};

static struct device_d dm9000_dev = {
	.id	  = -1,
	.name     = "dm9000",
	.map_base = 0x16000000,
	.size     = 8,
	.platform_data = &dm9000_data,
};

struct gpio_led leds[] = {
	{
		.gpio = 32 + 21,
	}, {
		.gpio = 32 + 22,
	}, {
		.gpio = 32 + 23,
	}, {
		.gpio = 32 + 24,
	},
};

static int scb9328_devices_init(void)
{
	int i;

	imx_gpio_mode(PA23_PF_CS5);
	imx_gpio_mode(GPIO_PORTB | GPIO_GPIO | GPIO_OUT | 21);
	imx_gpio_mode(GPIO_PORTB | GPIO_GPIO | GPIO_OUT | 22);
	imx_gpio_mode(GPIO_PORTB | GPIO_GPIO | GPIO_OUT | 23);
	imx_gpio_mode(GPIO_PORTB | GPIO_GPIO | GPIO_OUT | 24);

	for (i = 0; i < ARRAY_SIZE(leds); i++)
		led_gpio_register(&leds[i]);

/* CS3 becomes CS3 by clearing reset default bit 1 in FMCR */
	FMCR = 0x1;

	CS0U = 0x000F2000;
	CS0L = 0x11110d01;

	CS1U = 0x000F0a00;
	CS1L = 0x11110601;
	CS2U = 0x0;
	CS2L = 0x0;
	CS3U = 0x000FFFFF;
	CS3L = 0x00000303;
	CS4U = 0x000F0a00;
	CS4L = 0x11110301;
	CS5U = 0x00008400;
	CS5L = 0x00000D03;

	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&dm9000_dev);

	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");
	protect_file("/dev/env0", 1);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x08000100);
	armlinux_set_architecture(MACH_TYPE_SCB9328);

	return 0;
}

device_initcall(scb9328_devices_init);

static struct device_d scb9328_serial_device = {
	.id	  = -1,
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
};

static int scb9328_console_init(void)
{
	/* init gpios for serial port */
	imx_gpio_mode(PC11_PF_UART1_TXD);
	imx_gpio_mode(PC12_PF_UART1_RXD);

	register_device(&scb9328_serial_device);
	return 0;
}

console_initcall(scb9328_console_init);

