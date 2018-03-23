/*
 * Copyright (C) 2012 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <bootsource.h>
#include <environment.h>
#include <fcntl.h>
#include <platform_data/eth-fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <mci.h>

#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>
#include <mach/imx5.h>
#include <mach/bbu.h>

#include <asm/armlinux.h>
#include <io.h>
#include <asm/mmu.h>

static int tx53_devices_init(void)
{
	const char *envdev;
	uint32_t flag_nand = 0;
	uint32_t flag_mmc = 0;

	if (!of_machine_is_compatible("karo,tx53"))
		return 0;

	barebox_set_model("Ka-Ro TX53");
	barebox_set_hostname("tx53");

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		devfs_add_partition("mmc0", 0x00000, SZ_512K,
				DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("mmc0", SZ_512K, SZ_1M,
				DEVFS_PARTITION_FIXED, "env0");
		envdev = "MMC";
		flag_mmc |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	case BOOTSOURCE_NAND:
	default:
		devfs_add_partition("nand0", 0x00000, 0x80000,
				DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", 0x80000, 0x100000,
				DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		envdev = "NAND";
		flag_nand |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	}

	armlinux_set_architecture(MACH_TYPE_TX53);

	/* rev xx30 can boot from nand or USB */
	imx53_bbu_internal_nand_register_handler("nand-xx30",
						flag_nand, SZ_512K);

	/* rev 1011 can boot from MMC/SD, other bootsource currently unknown */
	imx53_bbu_internal_mmc_register_handler("mmc-1011",
						"/dev/mmc0", flag_mmc);

	if (of_machine_is_compatible("karo,tx53-1011"))
		imx53_init_lowlevel(1000);

	printf("Using environment in %s\n", envdev);

	return 0;
}
device_initcall(tx53_devices_init);
