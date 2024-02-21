// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Rouven Czerwinski, Pengutronix
 */
#define pr_fmt(fmt) "tqma6ul: " fmt

#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <mach/imx/generic.h>
#include <mach/imx/bbu.h>
#include <of.h>
#include <string.h>
#include <linux/clk.h>

static int mba6ulx_probe(struct device *dev)
{
	int flags;
	struct clk *clk;

	clk = clk_lookup("enet_ref_125m");
	if (IS_ERR(clk))
		pr_err("Cannot find enet_ref_125m: %pe\n", clk);
	else
		clk_enable(clk);

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
MODULE_DEVICE_TABLE(of, mba6ulx_of_match);

static struct driver mba6ulx_board_driver = {
	.name = "board-mba6ulx",
	.probe = mba6ulx_probe,
	.of_compatible = mba6ulx_of_match,
};
device_platform_driver(mba6ulx_board_driver);
