// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Sascha Hauer, Pengutronix

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <deep-probe.h>

static int imx6sx_udoneo_probe(struct device *dev)
{
	barebox_set_hostname("mx6sx-udooneo");

	return 0;
}

static const struct of_device_id imx6sx_udoneo_of_match[] = {
	{ .compatible = "fsl,imx6sx-udoo-neo" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(imx6sx_udoneo_of_match);

static struct driver imx6sx_udoneo_driver = {
	.name = "board-udoo-neo",
	.probe = imx6sx_udoneo_probe,
	.of_compatible = imx6sx_udoneo_of_match,
};
postcore_platform_driver(imx6sx_udoneo_driver);
