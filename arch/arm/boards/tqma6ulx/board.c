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
#include <asm/optee.h>
#include <asm-generic/memory_layout.h>

#include "tqma6ulx.h"

static const struct of_device_id mba6ulx_of_match[] = {
	{ .compatible = "tq,imx6ul-tqma6ul2l" },
	{ .compatible = "tq,imx6ul-tqma6ul2" },
	{ .compatible = "tq,imx6ull-tqma6ull2" },
	{ .compatible = "tq,imx6ull-tqma6ull2l" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mba6ulx_of_match);

#ifdef CONFIG_FIRMWARE_TQMA6UL_OPTEE

static int mba6ulx_optee_fixup(void)
{
	struct device_node *overlay;
	struct fdt_header *fdt;
	struct device_node *root = of_get_root_node();
	int ret;

	if (!of_match_node(mba6ulx_of_match, root))
		return 0;

	fdt = (void*)OPTEE_OVERLAY_LOCATION;
	overlay = of_unflatten_dtb(fdt, INT_MAX);

	if (IS_ERR(overlay))
		return PTR_ERR(overlay);

	/* register the overlay for fixing up the kernel device tree */
	ret = of_register_overlay(overlay);
	if (ret) {
		printf("cannot apply oftree overlay: %s\n", strerror(-ret));
		goto err;
	}

	/*
	 * Apply the overlay to the live tree to enable OP-TEE support
	 * for barebox and to reserve the SDRAM regions occupied by
	 * OP-TEE
	 */
	of_overlay_apply_tree(root, overlay);

	return 0;
err:
	of_delete_node(overlay);

	return ret;
}
postcore_initcall(mba6ulx_optee_fixup);

#endif

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
	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx6_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", flags);

	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx6_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc0", flags);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	return 0;
}

static struct driver mba6ulx_board_driver = {
	.name = "board-mba6ulx",
	.probe = mba6ulx_probe,
	.of_compatible = mba6ulx_of_match,
};
device_platform_driver(mba6ulx_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(mba6ulx_of_match);
