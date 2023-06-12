// SPDX-License-Identifier: GPL-2.0-only
/* *
 * Copyright (C) 2017 Lucas Stach <l.stach@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>

#define MC_VIDEO_PROTECT_SIZE_MB	0x64c
#define MC_VIDEO_PROTECT_REG_CTRL	0x650
#define  MC_VIDEO_PROTECT_REG_WR_DIS	(1 << 0)

static int tegra124_mc_of_fixup(struct device_node *root, void *context)
{
	struct device_node *np;

	np = of_find_compatible_node(root, NULL, "nvidia,gk20a");
	if (np)
		of_device_enable(np);

	return 0;
}

static int tegra124_mc_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	base = IOMEM(iores->start);

	/* disable video protect region */
	writel(0 , base + MC_VIDEO_PROTECT_SIZE_MB);
	writel(MC_VIDEO_PROTECT_REG_WR_DIS, base + MC_VIDEO_PROTECT_REG_CTRL);
	readl(base + MC_VIDEO_PROTECT_REG_CTRL); /* readback to flush */

	return of_register_fixup(tegra124_mc_of_fixup, NULL);
}

static __maybe_unused struct of_device_id tegra124_mc_dt_ids[] = {
	{
		.compatible = "nvidia,tegra124-mc",
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, tegra124_mc_dt_ids);

static struct driver tegra124_mc_driver = {
	.name		= "tegra124-mc",
	.of_compatible	= DRV_OF_COMPAT(tegra124_mc_dt_ids),
	.probe		= tegra124_mc_probe,
};

device_platform_driver(tegra124_mc_driver);
