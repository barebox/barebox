// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012 Sascha Hauer, Pengutronix

#include <common.h>
#include <bootsource.h>
#include <environment.h>
#include <fcntl.h>
#include <platform_data/eth-fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <mci.h>

#include <asm/mach-types.h>

#include <mach/imx/imx53-regs.h>
#include <mach/imx/iomux-mx53.h>
#include <mach/imx/devices-imx53.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx-nand.h>
#include <mach/imx/iim.h>
#include <mach/imx/imx5.h>
#include <mach/imx/bbu.h>

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
