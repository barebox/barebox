/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Phytec Messtechnik GmbH, Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <init.h>
#include <of.h>
#include <linux/err.h>

static int ti_sysc_probe(struct device *dev)
{
	int ret;

	ret = of_platform_populate(dev->of_node,
					of_default_bus_match_table, dev);
	if (ret)
		dev_err(dev, "%s fail to create devices.\n",
					dev->of_node->full_name);
	return ret;
};

static struct of_device_id ti_sysc_dt_ids[] = {
	{ .compatible = "ti,sysc-omap4",},
	{ .compatible = "ti,sysc-omap4-simple",},
	{ .compatible = "ti,sysc-omap4-timer",},
	{ .compatible = "ti,sysc-omap2",},
	{ },
};
MODULE_DEVICE_TABLE(of, ti_sysc_dt_ids);

static struct driver ti_sysc_driver = {
	.name = "ti-sysc",
	.probe = ti_sysc_probe,
	.of_compatible = DRV_OF_COMPAT(ti_sysc_dt_ids),
};

postcore_platform_driver(ti_sysc_driver);
