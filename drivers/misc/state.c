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
	struct device_node *partition_node;
	struct state *state;
	const char *alias;
	const char *backend_type = NULL;
	int len, ret;
	const char *of_path;
	char *path;

	if (!np)
		return -EINVAL;

	alias = of_alias_get(np);
	if (!alias)
		alias = "state";

	state = state_new_from_node(alias, np);
	if (IS_ERR(state))
		return PTR_ERR(state);

	of_path = of_get_property(np, "backend", &len);
	if (!of_path) {
		ret = -ENODEV;
		goto out_release;
	}

	/* guess if of_path is a path, not a phandle */
	if (of_path[0] == '/' && len > 1) {
		ret = of_find_path(np, "backend", &path, 0);
		if (ret)
			goto out_release;
	} else {
		struct device_d *dev;
		struct cdev *cdev;

		partition_node = of_parse_phandle(np, "backend", 0);
		if (!partition_node) {
			ret = -ENODEV;
			goto out_release;
		}

		dev = of_find_device_by_node(partition_node);
		if (!list_is_singular(&dev->cdevs)) {
			ret = -ENODEV;
			goto out_release;
		}

		cdev = list_first_entry(&dev->cdevs, struct cdev, devices_list);
		if (!cdev) {
			ret = -ENODEV;
			goto out_release;
		}

		path = asprintf("/dev/%s", cdev->name);
		of_path = partition_node->full_name;
	}

	ret = of_property_read_string(np, "backend-type", &backend_type);
	if (ret) {
		goto out_free;
	} else if (!strcmp(backend_type, "raw")) {
		ret = state_backend_raw_file(state, of_path, path, 0, 0);
	} else if (!strcmp(backend_type, "dtb")) {
		ret = state_backend_dtb_file(state, of_path, path);
	} else {
		dev_warn(dev, "invalid backend type: %s\n", backend_type);
		ret = -ENODEV;
		goto out_free;
	}

	if (ret)
		goto out_free;

	dev_info(dev, "backend: %s, path: %s, of_path: %s\n", backend_type, path, of_path);
	free(path);

	state_load(state);
	return 0;

 out_free:
	free(path);
 out_release:
	state_release(state);
	return ret;
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
