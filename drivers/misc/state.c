// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <of.h>
#include <state.h>

#include <linux/err.h>

static int state_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct state *state;
	bool readonly = false;
	int ret;

	state = state_new_from_node(np, readonly);
	if (IS_ERR(state)) {
		int ret = PTR_ERR(state);
		if (ret == -ENODEV)
			ret = -EPROBE_DEFER;
		return ret;
	}

	ret = state_load(state);
	if (ret == -ENOMEDIUM)
		dev_info(dev, "Fresh state detected, continuing with defaults\n");
	else if (ret)
		dev_warn(dev, "Failed to load persistent state, continuing with defaults, %d\n",
			 ret);

	return 0;
}

static __maybe_unused struct of_device_id state_ids[] = {
	{
		.compatible = "barebox,state",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, state_ids);

static struct driver state_driver = {
	.name = "state",
	.probe = state_probe,
	.of_compatible = DRV_OF_COMPAT(state_ids),
};
device_platform_driver(state_driver);
