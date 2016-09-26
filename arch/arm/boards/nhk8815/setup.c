/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
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
#include <init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <partition.h>
#include <nand.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <io.h>
#include <envfs.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <mach/nand.h>
#include <mach/fsmc.h>

static int nhk8815_nand_init(void)
{
	/* FSMC setup for nand chip select (8-bit nand in 8815NHK) */
	writel(0x0000000E, FSMC_PCR(0));
	writel(0x000D0A00, FSMC_PMEM(0));
	writel(0x00100A00, FSMC_PATT(0));

	/* enable access to the chip select area */
	writel(readl(FSMC_PCR(0)) | 0x04, FSMC_PCR(0));

	return 0;
}

static struct nomadik_nand_platform_data nhk8815_nand_data = {
	.init		= nhk8815_nand_init,
};

static struct resource nhk8815_nand_resources[] = {
	{
		.name	= "nand_addr",
		.start	= NAND_IO_ADDR,
		.end	= NAND_IO_ADDR + 0xfff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "nand_cmd",
		.start	= NAND_IO_CMD,
		.end	= NAND_IO_CMD + 0xfff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "nand_data",
		.start	= NAND_IO_DATA,
		.end	= NAND_IO_DATA + 0xfff,
		.flags	= IORESOURCE_MEM,
	}
};

static struct device_d nhk8815_nand_device = {
	.id		= DEVICE_ID_DYNAMIC,
	.name		= "nomadik_nand",
	.num_resources	= ARRAY_SIZE(nhk8815_nand_resources),
	.resource	= nhk8815_nand_resources,
	.platform_data	= &nhk8815_nand_data,
};

static int nhk8815_mem_init(void)
{
	st8815_add_device_sdram(64 * 1024 *1024);

	return 0;
}
mem_initcall(nhk8815_mem_init);

static int nhk8815_devices_init(void)
{
	writel(0xC37800F0, NOMADIK_GPIO1_BASE + 0x20);
	writel(0x00000000, NOMADIK_GPIO1_BASE + 0x24);
	writel(0x00000000, NOMADIK_GPIO1_BASE + 0x28);
	writel(readl(NOMADIK_SRC_BASE) | 0x8000, NOMADIK_SRC_BASE);

	/* Set up SMCS1 for Ethernet: sram-like, enabled, timing values */
	writel(0x0000305b, FSMC_BCR(1));
	writel(0x00033f33, FSMC_BTR(1));

	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL, 0x34000300, 16,
			   IORESOURCE_MEM, NULL);

	platform_device_register(&nhk8815_nand_device);

	armlinux_set_architecture(MACH_TYPE_NOMADIK);

	devfs_add_partition("nand0", 0x0000000, 0x040000, DEVFS_PARTITION_FIXED, "xloader_raw");
	devfs_add_partition("nand0", 0x0040000, 0x080000, DEVFS_PARTITION_FIXED, "meminit_raw");
	devfs_add_partition("nand0", 0x0080000, 0x200000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x7FE0000, 0x020000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_nhk8815);

	return 0;
}
device_initcall(nhk8815_devices_init);

static int nhk8815_console_init(void)
{
	barebox_set_model("Nomadik nhk8815");
	barebox_set_hostname("nhk8815");

	st8815_register_uart(1);

	return 0;
}

console_initcall(nhk8815_console_init);
