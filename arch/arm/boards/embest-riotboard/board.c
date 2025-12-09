// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Eric Bénard <eric@eukrea.com>
// SPDX-FileCopyrightText: 2013 Lucas Stach <l.stach@pengutronix.de>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <bootsource.h>
#include <common.h>
#include <environment.h>
#include <envfs.h>
#include <gpio.h>
#include <init.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx6.h>
#include <mach/imx/bbu.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <linux/sizes.h>
#include <linux/phy.h>
#include <deep-probe.h>

static int riotboard_probe(struct device *dev)
{
	imx6_bbu_internal_mmcboot_register_handler("emmc", "/dev/mmc3",
			BBU_HANDLER_FLAG_DEFAULT);
	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc2", 0);

	barebox_set_hostname("riotboard");

	return 0;
}

static const struct of_device_id riotboard_of_match[] = {
	{ .compatible = "riot,imx6s-riotboard"},
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(riotboard_of_match);

static struct driver riotboard_board_driver = {
	.name = "board-riotboard",
	.probe = riotboard_probe,
	.of_compatible = DRV_OF_COMPAT(riotboard_of_match),
};
device_platform_driver(riotboard_board_driver);
