/*
 * (C) 2007 konzeptpark, Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  @brief This file contains ...
 *
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <fec.h>
#include <environment.h>
#include <mach/mcf54xx-regs.h>
#include <mach/clocks.h>
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
	.map_base = MCF_FEC_ADDR(0),
	.size     = MCF_FEC_SIZE(0),	   /* area size */
	.platform_data	= &fec_info,
};
static struct device_d network_dev1 =
{
	.name     = "fec_mcf54xx",
	.map_base = MCF_FEC_ADDR(1),
	.size     = MCF_FEC_SIZE(1),	   /* area size */
	.platform_data	= &fec_info,
};

/*
 * 128MiB of SDRAM, data width is 32 bit
 */
static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev =
{
	.name     = "mem",
	.map_base = CFG_SDRAM_ADDRESS,
	.size     = CFG_SDRAM_SIZE,
	.platform_data = &ram_pdata,
};

static int mcfv4e_devices_init(void)
{
	printf("FIXME - setup board devices...\n");

	register_device(&cfi_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x80000, PARTITION_FIXED, "self0");	/* ourself */
	devfs_add_partition("nor0", 0x80000, 0x40000, PARTITION_FIXED, "env0");	/* environment */
	protect_file("/dev/env0", 1);

	register_device(&network_dev0);
	//register_device(&network_dev1);

	register_device(&sdram_dev);

	return 0;
}

device_initcall(mcfv4e_devices_init);

static struct device_d mcfv4e_serial_device =
{
	.name     = "mcfv4e_serial",
	.map_base = 1 + CFG_EARLY_UART_PORT,
	.size     = 16 * 1024,
};

static int mcfv4e_console_init(void)
{
	/* init gpios for serial port */

	/* Already set in lowlevel_init.c */

	register_device(&mcfv4e_serial_device);
	return 0;
}

console_initcall(mcfv4e_console_init);
