// SPDX-License-Identifier: GPL-2.0
/*
 * Functions for working with device tree overlays
 *
 * Copyright (C) 2012 Pantelis Antoniou <panto@antoniou-consulting.com>
 * Copyright (C) 2012 Texas Instruments Inc.
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */
#define pr_fmt(fmt) "of_overlay: " fmt

#include <common.h>
#include <of.h>
#include <errno.h>

static struct device_node *find_target(struct device_node *root,
				       struct device_node *fragment)
{
	struct device_node *node;
	const char *path;
	u32 phandle;
	int ret;

	ret = of_property_read_u32(fragment, "target", &phandle);
	if (!ret) {
		node = of_find_node_by_phandle_from(phandle, root);
		if (!node)
			pr_err("fragment %pOF: phandle 0x%x not found\n",
			       fragment, phandle);
		return node;
	}

	ret = of_property_read_string(fragment, "target-path", &path);
	if (!ret) {
		node = of_find_node_by_path_from(root, path);
		if (!node)
			pr_err("fragment %pOF: path '%s' not found\n",
			       fragment, path);
		return node;
	}

	pr_err("fragment %pOF: no target property\n", fragment);

	return NULL;
}

static int of_overlay_apply(struct device_node *target,
			    const struct device_node *overlay)
{
	struct device_node *child;
	struct device_node *target_child;
	struct property *prop;
	int err;

	if (target == NULL || overlay == NULL)
		return -EINVAL;

	list_for_each_entry(prop, &overlay->properties, list) {
		if (of_prop_cmp(prop->name, "name") == 0)
			continue;

		err = of_set_property(target, prop->name, prop->value,
				      prop->length, true);
		if (err)
			return err;
	}

	for_each_child_of_node(overlay, child) {
		target_child = of_get_child_by_name(target, child->name);
		if (!target_child)
			target_child = of_new_node(target, child->name);
		if (!target_child)
			return -ENOMEM;

		err = of_overlay_apply(target_child, child);
		if (err)
			return err;
	}

	return 0;
}

static char *of_overlay_fix_path(struct device_node *root,
				 struct device_node *overlay, const char *path)
{
	struct device_node *fragment;
	struct device_node *target;
	const char *path_tail;
	const char *prefix;

	fragment = of_find_node_by_path_from(overlay, path);
	while ((fragment = of_get_parent(fragment)) != NULL) {
		if (of_get_child_by_name(fragment, "__overlay__"))
			break;
	}
	if (!fragment)
		return NULL;

	target = find_target(root, fragment);
	if (!target)
		return NULL;

	prefix = of_get_child_by_name(fragment, "__overlay__")->full_name;
	path_tail = path + strlen(prefix);

	return basprintf("%s%s", target->full_name, path_tail);
}

static int of_overlay_apply_symbols(struct device_node *root,
				    struct device_node *overlay)
{
	const char *old_path;
	char *new_path;
	struct property *prop;
	struct device_node *root_symbols;
	struct device_node *overlay_symbols;

	root_symbols = of_get_child_by_name(root, "__symbols__");
	if (!root_symbols)
		return -EINVAL;

	overlay_symbols = of_get_child_by_name(overlay, "__symbols__");
	if (!overlay_symbols)
		return -EINVAL;

	list_for_each_entry(prop, &overlay_symbols->properties, list) {
		if (of_prop_cmp(prop->name, "name") == 0)
			continue;

		old_path = of_property_get_value(prop);
		new_path = of_overlay_fix_path(root, overlay, old_path);

		pr_debug("add symbol %s with new path %s\n",
			 prop->name, new_path);
		of_property_write_string(root_symbols, prop->name, new_path);
	}

	return 0;
}

static int of_overlay_apply_fragment(struct device_node *root,
				     struct device_node *fragment)
{
	struct device_node *target;
	struct device_node *overlay;

	overlay = of_get_child_by_name(fragment, "__overlay__");
	if (!overlay)
		return 0;

	target = find_target(root, fragment);
	if (!target)
		return -EINVAL;

	return of_overlay_apply(target, overlay);
}

/**
 * Apply the overlay on the passed devicetree root
 * @root: the devicetree onto which the overlay will be applied
 * @overlay: the devicetree to apply as an overlay
 */
int of_overlay_apply_tree(struct device_node *root,
			  struct device_node *overlay)
{
	struct device_node *resolved;
	struct device_node *fragment;
	int err;

	resolved = of_resolve_phandles(root, overlay);
	if (!resolved)
		return -EINVAL;

	/* Copy symbols from resolved overlay to base device tree */
	err = of_overlay_apply_symbols(root, resolved);
	if (err)
		pr_warn("failed to copy symbols from overlay");

	/* Copy nodes and properties from resolved overlay to root */
	for_each_child_of_node(resolved, fragment) {
		err = of_overlay_apply_fragment(root, fragment);
		if (err)
			pr_warn("failed to apply %s", fragment->name);
	}

	of_delete_node(resolved);

	return err;
}

static int of_overlay_fixup(struct device_node *root, void *data)
{
	struct device_node *overlay = data;

	return of_overlay_apply_tree(root, overlay);
}

/**
 * Iterate the overlay and call process for each fragment
 *
 * If process() fails for any fragment, the function will stop to process
 * other fragments and return the error of the failed process() call.
 */
int of_process_overlay(struct device_node *root,
		       struct device_node *overlay,
		       int (*process)(struct device_node *target,
				      struct device_node *overlay, void *data),
		       void *data)
{
	struct device_node *fragment;
	int err = 0;

	for_each_child_of_node(overlay, fragment) {
		struct device_node *ovl;
		struct device_node *target;

		ovl = of_get_child_by_name(fragment, "__overlay__");
		if (!ovl)
			continue;

		target = find_target(root, fragment);
		if (!target)
			continue;

		err = process(target, ovl, data);
		if (err) {
			pr_warn("failed to process overlay for %s\n",
				target->name);
			break;
		}
	}

	return err;
}

/**
 * Register a devicetree overlay
 *
 * The overlay is not applied to the live device tree, but registered as fixup
 * for the fixed up device tree. Therefore, drivers relying on the overlay
 * must use the fixed device tree.
 *
 * The fixup takes ownership of the overlay.
 */
int of_register_overlay(struct device_node *overlay)
{
	return of_register_fixup(of_overlay_fixup, overlay);
}
