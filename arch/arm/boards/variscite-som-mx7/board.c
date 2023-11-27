// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Roland Hieber, Pengutronix <rhi@pengutronix.de>

#include <common.h>
#include <deep-probe.h>
#include <mach/imx/bbu.h>

static int var_som_mx7_probe(struct device_d *dev)
{
	imx7_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", BBU_HANDLER_FLAG_DEFAULT);
	return 0;
}

static const struct of_device_id var_som_mx7_of_match[] = {
	{ .compatible = "variscite,var-som-mx7" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(var_som_mx7_of_match);

static struct driver_d var_som_mx7_board_driver = {
	.name = "board-var-som-mx7",
	.probe = var_som_mx7_probe,
	.of_compatible = DRV_OF_COMPAT(var_som_mx7_of_match),
};
postcore_platform_driver(var_som_mx7_board_driver);
