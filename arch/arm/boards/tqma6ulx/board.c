// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Rouven Czerwinski, Pengutronix
 */

#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <mach/generic.h>
#include <mach/bbu.h>
#include <of.h>
#include <string.h>

static int mba6ulx_probe(struct device_d *dev)
{
	int flags;

	/* the bootloader is stored in one of the two boot partitions */
	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx6_bbu_internal_mmc_register_handler("SD", "/dev/mmc0.barebox", flags);

	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx6_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc1", flags);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	return 0;
}

static const struct of_device_id mba6ulx_of_match[] = {
	{ .compatible = "tq,mba6ulx" },
	{ /* sentinel */ },
};

static struct driver_d mba6ulx_board_driver = {
	.name = "board-mba6ulx",
	.probe = mba6ulx_probe,
	.of_compatible = mba6ulx_of_match,
};
device_platform_driver(mba6ulx_board_driver);
