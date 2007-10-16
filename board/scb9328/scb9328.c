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
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0x10000000,
	.size     = 16 * 1024 * 1024,
};

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x08000000,
	.size     = 16 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct device_d dm9000_dev = {
	.name     = "dm9000",
	.id       = "eth0",

	.type     = DEVICE_TYPE_ETHER,
};

static int scb9328_devices_init(void) {

	/* adjust chipselects */
	GPR(0) = 0x00800000;
	GIUS(0) = 0x0043fffe;

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

	dev_add_partition(&cfi_dev, 0x00000, 0x20000, "self");
	dev_add_partition(&cfi_dev, 0x40000, 0x20000, "env");
	dev_protect(&cfi_dev, 0x20000, 0, 1);

	return 0;
}

device_initcall(scb9328_devices_init);

static struct device_d scb9328_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
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

