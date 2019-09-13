// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */
#include <common.h>
#include <firmware.h>
#include <of.h>

struct overlay_info {
	const char *firmware_path;
};

static struct firmware_mgr *of_node_get_mgr(struct device_node *np)
{
	struct device_node *mgr_node;

	do {
		if (of_device_is_compatible(np, "fpga-region")) {
			mgr_node = of_parse_phandle(np, "fpga-mgr", 0);
			if (mgr_node)
				return firmwaremgr_find_by_node(mgr_node);
		}
	} while ((np = of_get_parent(np)) != NULL);

	return NULL;
}

static int load_firmware(struct device_node *target,
			 struct device_node *fragment, void *data)
{
	struct overlay_info *info = data;
	const char *firmware_name;
	const char *firmware_path = info->firmware_path;
	char *firmware;
	int err;
	struct firmware_mgr *mgr;

	err = of_property_read_string(fragment,
				      "firmware-name", &firmware_name);
	/* Nothing to do if property does not exist. */
	if (err == -EINVAL)
		return 0;
	else if (err)
		return -EINVAL;

	mgr = of_node_get_mgr(target);
	if (!mgr)
		return -EINVAL;

	firmware = basprintf("%s/%s", firmware_path, firmware_name);
	if (!firmware)
		return -ENOMEM;

	err = firmwaremgr_load_file(mgr, firmware);

	free(firmware);

	return err;
}

int of_firmware_load_overlay(struct device_node *overlay, const char *path)
{
	struct overlay_info info = {
		.firmware_path = path,
	};
	int err;
	struct device_node *root;
	struct device_node *resolved;
	struct device_node *ovl;

	root = of_get_root_node();
	/*
	 * If we cannot resolve the symbols in the overlay, ensure that the
	 * overlay does depend on firmware to be loaded.
	 */
	resolved = of_resolve_phandles(root, overlay);
	ovl = resolved ? resolved : overlay;

	err = of_process_overlay(root, ovl,
				 load_firmware, &info);

	if (resolved)
		of_delete_node(resolved);

	return err;
}
