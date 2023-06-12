// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Generic Syscon Poweroff Driver
 *
 * Copyright (c) 2015, National Instruments Corp.
 * Author: Moritz Fischer <moritz.fischer@ettus.com>
 */

#include <common.h>
#include <init.h>
#include <poweroff.h>
#include <mfd/syscon.h>

static struct regmap *map;
static u32 offset;
static u32 value;
static u32 mask;

static void syscon_poweroff(struct poweroff_handler *handler)
{
	/* Issue the poweroff */
	regmap_update_bits(map, offset, mask, value);

	mdelay(1000);

	pr_emerg("Unable to poweroff system\n");
}

static int syscon_poweroff_probe(struct device *dev)
{
	int mask_err, value_err;

	map = syscon_regmap_lookup_by_phandle(dev->of_node, "regmap");
	if (IS_ERR(map)) {
		dev_err(dev, "unable to get syscon");
		return PTR_ERR(map);
	}

	if (of_property_read_u32(dev->of_node, "offset", &offset)) {
		dev_err(dev, "unable to read 'offset'");
		return -EINVAL;
	}

	value_err = of_property_read_u32(dev->of_node, "value", &value);
	mask_err = of_property_read_u32(dev->of_node, "mask", &mask);
	if (value_err && mask_err) {
		dev_err(dev, "unable to read 'value' and 'mask'");
		return -EINVAL;
	}

	if (value_err) {
		/* support old binding */
		value = mask;
		mask = 0xFFFFFFFF;
	} else if (mask_err) {
		/* support value without mask*/
		mask = 0xFFFFFFFF;
	}

	poweroff_handler_register_fn(syscon_poweroff);

	return 0;
}

static const struct of_device_id syscon_poweroff_of_match[] = {
	{ .compatible = "syscon-poweroff" },
	{}
};
MODULE_DEVICE_TABLE(of, syscon_poweroff_of_match);

static struct driver syscon_poweroff_driver = {
	.name = "syscon-poweroff",
	.of_compatible = syscon_poweroff_of_match,
	.probe = syscon_poweroff_probe,
};

coredevice_platform_driver(syscon_poweroff_driver);
