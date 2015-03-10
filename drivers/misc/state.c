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
	const char *alias;
	const char *backend_type = NULL;
	int ret;
	char *path;

	if (!np)
		return -EINVAL;

	alias = of_alias_get(np);
	if (!alias)
		alias = "state";

	state = state_new_from_node(alias, np);
	if (IS_ERR(state))
		return PTR_ERR(state);

	ret = of_find_path(np, "backend", &path);
	if (ret)
		return ret;

	dev_info(dev, "outpath: %s\n", path);

	ret = of_property_read_string(np, "backend-type", &backend_type);
	if (ret)
		return ret;
	else if (!strcmp(backend_type, "raw"))
		ret = state_backend_raw_file(state, path, 0, 0);
	else if (!strcmp(backend_type, "dtb"))
		ret = state_backend_dtb_file(state, path);
	else
		dev_warn(dev, "invalid backend type: %s\n", backend_type);

	if (ret)
		return ret;

	state_load(state);

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
