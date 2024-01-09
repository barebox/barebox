// SPDX-License-Identifier: GPL-2.0-only

#include <deep-probe.h>
#include <bootsource.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <asm/memory.h>
#include <mach/layerscape/layerscape.h>
#include <mach/layerscape/bbu.h>
#include <linux/sizes.h>

static int ls1028ardb_probe(struct device *dev)
{
	unsigned long sd_bbu_flags = 0;
	unsigned long emmc_bbu_flags = 0;

	arm_add_mem_device("ram1", LS1028A_DDR_SDRAM_HIGHMEM_BASE, SZ_2G);

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 0) {
		sd_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-sd");
	} else {
		emmc_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-emmc");
	}

	ls1028a_bbu_mmc_register_handler("sd", "/dev/mmc0.barebox", sd_bbu_flags);
	ls1028a_bbu_mmc_register_handler("emmc", "/dev/mmc1.barebox", emmc_bbu_flags);

	return 0;
}

static const struct of_device_id ls1028a_of_match[] = {
	{ .compatible = "fsl,ls1028a-rdb" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(ls1028a_of_match);

static struct driver ls1028ardb_board_driver = {
	.name = "ls1028a-rdb",
	.probe = ls1028ardb_probe,
	.of_compatible = ls1028a_of_match,
};
device_platform_driver(ls1028ardb_board_driver);
