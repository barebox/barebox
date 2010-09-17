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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <mach/nand.h>
#include <mach/fsmc.h>

static struct device_d nhk8815_network_dev = {
	.id = -1,
	.name = "smc91c111",
	.map_base = 0x34000300,
	.size = 16,
};

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
	.addr_va	= NAND_IO_ADDR,
	.cmd_va		= NAND_IO_CMD,
	.data_va	= NAND_IO_DATA,
	.options	= NAND_COPYBACK | NAND_CACHEPRG | NAND_NO_PADDING \
			| NAND_NO_READRDY | NAND_NO_AUTOINCR,
	.init		= nhk8815_nand_init,
};

static struct device_d nhk8815_nand_device = {
	.id		= -1,
	.name		= "nomadik_nand",
	.platform_data	= &nhk8815_nand_data,
};

static int nhk8815_devices_init(void)
{
	st8815_add_device_sdram(64 * 1024 *1024);

	writel(0xC37800F0, NOMADIK_GPIO1_BASE + 0x20);
	writel(0x00000000, NOMADIK_GPIO1_BASE + 0x24);
	writel(0x00000000, NOMADIK_GPIO1_BASE + 0x28);
	writel(readl(NOMADIK_SRC_BASE) | 0x8000, NOMADIK_SRC_BASE);

	/* Set up SMCS1 for Ethernet: sram-like, enabled, timing values */
	writel(0x0000305b, FSMC_BCR(1));
	writel(0x00033f33, FSMC_BTR(1));

	register_device(&nhk8815_network_dev);

	register_device(&nhk8815_nand_device);

	armlinux_set_architecture(MACH_TYPE_NOMADIK);
	armlinux_set_bootparams((void *)(0x00000100));

	devfs_add_partition("nand0", 0x0000000, 0x040000, PARTITION_FIXED, "xloader_raw");
	devfs_add_partition("nand0", 0x0040000, 0x080000, PARTITION_FIXED, "meminit_raw");
	devfs_add_partition("nand0", 0x0080000, 0x200000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x7FE0000, 0x020000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	return 0;
}
device_initcall(nhk8815_devices_init);

static int nhk8815_console_init(void)
{
	st8815_register_uart(1);
	return 0;
}

console_initcall(nhk8815_console_init);
