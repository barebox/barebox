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
#include <asm/io.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

static struct resource cfi_resources[] = {
	[0] = {
		.start	= AT91_CHIPSELECT_0,
		.flags	= IORESOURCE_MEM,
	},
};

static struct device_d cfi_dev = {
	.id		= 0,
	.name		= "cfi_flash",
	.num_resources	= ARRAY_SIZE(cfi_resources),
	.resource	= cfi_resources,
};

static struct at91_ether_platform_data ether_pdata = {
	.flags = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

static int at91rm9200ek_devices_init(void)
{
	/*
	 * Correct IRDA resistor problem
	 * Set PA23_TXD in Output
	 */
	at91_set_gpio_output(AT91_PIN_PA23, 1);

	at91_add_device_sdram(64 * 1024 * 1024);
	at91_add_device_eth(&ether_pdata);
	register_device(&cfi_dev);

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
