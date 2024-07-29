// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Holger Assmann <h.assmann@pengutronix.de>
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>
#include <gpio.h>
#include <envfs.h>

static int tqma8mpxl_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 1) {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	return 0;
}

static const struct of_device_id tqma8mpxl_of_match[] = {
	{ .compatible = "tq,imx8mp-tqma8mpql" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(tqma8mpxl_of_match);

static struct driver tqma8mpxl_board_driver = {
	.name = "board-tqma8mpxl",
	.probe = tqma8mpxl_probe,
	.of_compatible = DRV_OF_COMPAT(tqma8mpxl_of_match),
};
device_platform_driver(tqma8mpxl_board_driver);
