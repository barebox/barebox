/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <of.h>
#include <state.h>

#include <linux/err.h>

static int state_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct state *state;
	bool readonly = false;

	state = state_new_from_node(np, NULL, 0, 0, readonly);
	if (IS_ERR(state)) {
		int ret = PTR_ERR(state);
		if (ret == -ENODEV)
			ret = -EPROBE_DEFER;
		return ret;
	}

	return 0;
}

static __maybe_unused struct of_device_id state_ids[] = {
	{
		.compatible = "barebox,state",
	}, {
		/* sentinel */
	}
};

static struct driver_d state_driver = {
	.name = "state",
	.probe = state_probe,
	.of_compatible = DRV_OF_COMPAT(state_ids),
};
device_platform_driver(state_driver);
