/*
 * Copyright (C) 2010 Marek Belisko <marek.belisko@open-nandra.com>
 *
 * Based on a9m2440.c board init by Juergen Beisert, Pengutronix
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

/**
 * @file
 * @brief mini2440 Specific Board Initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <nand.h>
#include <asm/io.h>
#include <mach/s3c24x0-iomap.h>
#include <mach/s3c24x0-nand.h>
#include <mach/s3c24xx-generic.h>
#include <dm9000.h>

static struct memory_platform_data ram_pdata = {
	.name		= "ram0",
	.flags		= DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name		= "mem",
	.map_base	= CS6_BASE,
	.size		= 64 * 1024 * 1024,
	.platform_data	= &ram_pdata,
};

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
};

static struct device_d nand_dev = {
	.name     = "s3c24x0_nand",
	.map_base = S3C24X0_NAND_BASE,
	.platform_data	= &nand_info,
};

/*
 * dm9000 network controller onboard
 */
static struct dm9000_platform_data dm9000_data = {
	.iobase   = CS4_BASE + 0x300,
	.iodata   = CS4_BASE + 0x304,
	.buswidth = DM9000_WIDTH_16,
	.srom     = 1,
};

static struct device_d dm9000_dev = {
	.name     = "dm9000",
	.map_base = CS4_BASE + 0x300,
	.size     = 8,
	.platform_data = &dm9000_data,
};

static int mini2440_devices_init(void)
{
	uint32_t reg;

	reg = readl(BWSCON);

	/* CS#4 to access the network controller */
	reg &= ~0x000f0000;
	reg |=  0x000d0000;	/* 16 bit */
	writel(0x1f4c, BANKCON4);

	writel(reg, BWSCON);

	/* release the reset signal to external devices */
	reg = readl(MISCCR);
	reg |= 0x10000;
	writel(reg, MISCCR);

	register_device(&nand_dev);
	register_device(&sdram_dev);
	register_device(&dm9000_dev);
#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_del_partition("self_raw");
	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", NULL);

	devfs_del_partition("env_raw");
	devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", NULL);
#endif
	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)sdram_dev.map_base + 0x100);
	armlinux_set_architecture(MACH_TYPE_MINI2440);

	return 0;
}

device_initcall(mini2440_devices_init);

#ifdef CONFIG_S3C24XX_NAND_BOOT
void __bare_init nand_boot(void)
{
	s3c24x0_nand_load_image((void *)TEXT_BASE, 256 * 1024, 0, 512);
}
#endif

static struct device_d mini2440_serial_device = {
	.name     = "s3c24x0_serial",
	.map_base = UART1_BASE,
	.size     = UART1_SIZE,
};

static int mini2440_console_init(void)
{
	register_device(&mini2440_serial_device);
	return 0;
}

console_initcall(mini2440_console_init);
