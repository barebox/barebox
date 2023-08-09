// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */
#include <common.h>
#include <firmware.h>
#include <of.h>

static struct firmware_mgr *of_node_get_mgr(struct device_node *np)
{
	struct device_node *mgr_node;

	do {
		mgr_node = of_parse_phandle_from(np, of_find_root_node(np),
						 "fpga-mgr", 0);
		if (mgr_node)
			return firmwaremgr_find_by_node(mgr_node);
	} while ((np = of_get_parent(np)) != NULL);

	return NULL;
}

struct fw_load_entry {
	struct firmware_mgr *mgr;
	char *firmware;
	struct list_head list;
};

static LIST_HEAD(fw_load_list);

static int load_firmware(struct device_node *target,
			 struct device_node *fragment, void *unused)
{
	const char *firmware_name;
	int err;
	struct firmware_mgr *mgr;
	struct fw_load_entry *fle;

	err = of_property_read_string(fragment,
				      "firmware-name", &firmware_name);
	/* Nothing to do if property does not exist. */
	if (err == -EINVAL)
		return 0;
	else if (err)
		return -EINVAL;

	if (!target)
		return -EINVAL;

	if (!of_device_is_compatible(target, "fpga-region"))
		return 0;

	mgr = of_node_get_mgr(target);
	if (!mgr)
		return -EINVAL;

	fle = xzalloc(sizeof(*fle));
	fle->mgr = mgr;
	fle->firmware = xstrdup(firmware_name);

	list_add_tail(&fle->list, &fw_load_list);

	return 0;
}

/*
 * The dt overlay API says that a "firmware-name" property found in an overlay
 * node compatible to "fpga-region" triggers loading of the firmware with the
 * name given in the "firmware-name" property.
 *
 * barebox applies overlays to the Kernel device tree as part of booting the
 * Kernel. When a firmware is needed for an overlay then it shall be loaded,
 * so that the Kernel already finds the firmware loaded.
 * 
 * In barebox overlays are applied as a of_fixup to the desired tree. The fixups
 * might be executed multiple times not only as part of booting the Kernel, but
 * also during of_diff command execution and other actions. It's not desired
 * that we (re-)load all firmwares each time this happens, so the process is
 * splitted up. During application of an overlay the needed firmwares are only
 * collected to a list, but not actually loaded. Only once it's clear we want to
 * boot with that device tree the firmwares are loaded by explicitly calling
 * of_overlay_load_firmware().
 */

/**
 * of_overlay_pre_load_firmware() - check overlay node for firmware to load
 * @root: The device tree to apply the overlay to
 * @overlay: The overlay
 *
 * This function checks the given overlay for firmware to load. If a firmware
 * is needed then it is not directly loaded, but instead added to a list of
 * firmware to be loaded. The firmware files on this list can then be loaded
 * with of_overlay_load_firmware().
 *
 * Return: 0 for success or negative error code otherwise
 */
int of_overlay_pre_load_firmware(struct device_node *root, struct device_node *overlay)
{
	return of_process_overlay(root, overlay, load_firmware, NULL);
}

/**
 * of_overlay_load_firmware() - load all firmware files
 *
 * This function loads all firmware files previously collected in
 * of_overlay_pre_load_firmware().
 *
 * Return: 0 when all firmware files could be loaded, negative error code
 * otherwise.
 */
int of_overlay_load_firmware(void)
{
	struct fw_load_entry *fle;
	int ret;

	list_for_each_entry(fle, &fw_load_list, list) {
		ret = firmwaremgr_load_file(fle->mgr, fle->firmware);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * of_overlay_load_firmware_clear() - Clear list of firmware files
 *
 * This function clears the list of firmware files.
 */
void of_overlay_load_firmware_clear(void)
{
	struct fw_load_entry *fle, *tmp;

	list_for_each_entry_safe(fle, tmp, &fw_load_list, list) {
		list_del(&fle->list);
		free(fle->firmware);
		free(fle);
	}
}
