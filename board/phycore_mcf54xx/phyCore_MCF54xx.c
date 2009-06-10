/*
 * (C) 2007 konzeptpark, Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of U-Boot V2.
 *
 * U-Boot V2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * U-Boot V2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with U-Boot V2.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file This file contains ...
 *
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <fec.h>
#include <environment.h>
#include <asm/arch/mcf54xx-regs.h>
#include <asm/arch/clocks.h>
#include <asm/io.h>
#include <partition.h>

/*
 *  Return board clock in MHz    FIXME move to clocks file
 */
ulong mcfv4e_get_bus_clk(void)
{
	return CFG_SYSTEM_CORE_CLOCK;
}

/*
 * Up to 64MiB NOR type flash, connected to
 * CS line 0, data width is 32 bit
 */
static struct device_d cfi_dev =
{
	.name     = "cfi_flash",
	.id       = "nor0",
	.map_base = CFG_FLASH_ADDRESS,
	.size     = CFG_FLASH_SIZE,
};

/*
 * Build in FastEthernetControllers (FECs)
 */
static struct fec_platform_data fec_info =
{
	.xcv_type = MII100,
};

static struct device_d network_dev0 =
{
	.name     = "fec_mcf54xx",
	.id       = "eth0",
	.map_base = MCF_FEC_ADDR(0),
	.size     = MCF_FEC_SIZE(0),	   /* area size */
	.platform_data	= &fec_info,
};
static struct device_d network_dev1 =
{
	.name     = "fec_mcf54xx",
	.id       = "eth1",
	.map_base = MCF_FEC_ADDR(1),
	.size     = MCF_FEC_SIZE(1),	   /* area size */
	.platform_data	= &fec_info,
};

/*
 * 128MiB of SDRAM, data width is 32 bit
 */
static struct device_d sdram_dev =
{
	.name     = "ram",
	.id       = "ram0",

	.map_base = CFG_SDRAM_ADDRESS,
	.size     = CFG_SDRAM_SIZE,
};

static int mcfv4e_devices_init(void)
{
	printf("FIXME - setup board devices...\n");

	register_device(&cfi_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x80000, PARTITION_FIXED, "self");	/* ourself */
	devfs_add_partition("nor0", 0x80000, 0x40000, PARTITION_FIXED, "env");	/* environment */
	dev_protect(&cfi_dev, 0xc0000, 0, 1);

	register_device(&network_dev0);
	//register_device(&network_dev1);

	register_device(&sdram_dev);

	return 0;
}

device_initcall(mcfv4e_devices_init);

static struct device_d mcfv4e_serial_device =
{
	.name     = "mcfv4e_serial",
	.id       = "cs0",
	.map_base = 1 + CFG_EARLY_UART_PORT,
	.size     = 16 * 1024,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int mcfv4e_console_init(void)
{
	/* init gpios for serial port */

	/* Already set in lowlevel_init.c */

	register_device(&mcfv4e_serial_device);
	return 0;
}

console_initcall(mcfv4e_console_init);
