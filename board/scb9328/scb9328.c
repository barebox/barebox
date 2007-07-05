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
#include <partition.h>

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
	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&dm9000_dev);

	dev_add_partition(&cfi_dev, 0x00000, 0x20000, "self");
	dev_add_partition(&cfi_dev, 0x40000, 0x20000, "env");

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

