// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Lucas Stach <dev@lynxeye.de>
 */

#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <mach/bbu.h>

static int mnt_reform_probe(struct device_d *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", emmc_bbu_flag);

	return 0;
}

static const struct of_device_id mnt_reform_of_match[] = {
	{ .compatible = "mntre,reform2"},
	{ /* sentinel */ },
};

static struct driver_d mnt_reform_board_driver = {
	.name = "board-mnt-reform",
	.probe = mnt_reform_probe,
	.of_compatible = DRV_OF_COMPAT(mnt_reform_of_match),
};
device_platform_driver(mnt_reform_board_driver);
