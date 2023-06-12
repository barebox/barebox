// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <linux/clk.h>

static int am335x_child_probe(struct device *dev)
{
	int ret;

	ret = of_platform_populate(dev->of_node, NULL, dev);
	if (ret)
		return ret;

	return 0;
}

static __maybe_unused struct of_device_id am335x_child_dt_ids[] = {
	{
		.compatible = "ti,am33xx-usb",
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, am335x_child_dt_ids);

static struct driver am335x_child_driver = {
	.name   = "am335x_child_probe",
	.probe  = am335x_child_probe,
	.of_compatible = DRV_OF_COMPAT(am335x_child_dt_ids),
};
device_platform_driver(am335x_child_driver);
