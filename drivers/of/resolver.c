// SPDX-License-Identifier: GPL-2.0
/*
 * Functions for dealing with DT resolution
 *
 * Copyright (C) 2012 Pantelis Antoniou <panto@antoniou-consulting.com>
 * Copyright (C) 2012 Texas Instruments Inc.
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */
#define pr_fmt(fmt) "of_resolver: " fmt

#include <common.h>
#include <of.h>
#include <errno.h>

/**
 * Recursively update phandles in overlay by adding delta
 */
static void adjust_overlay_phandles(struct device_node *overlay, int delta)
{
	struct device_node *child;
	struct property *prop;

	if (overlay->phandle != 0)
		overlay->phandle += delta;

	list_for_each_entry(prop, &overlay->properties, list) {
		if (of_prop_cmp(prop->name, "phandle") != 0 &&
		    of_prop_cmp(prop->name, "linux,phandle") != 0)
			continue;
		if (prop->length < 4)
			continue;

		be32_add_cpu(prop->value, delta);
	}

	for_each_child_of_node(overlay, child)
		adjust_overlay_phandles(child, delta);
}

/**
 * Update all unresolved phandles in the overlay using prop_fixup
 *
 * prop_fixup contains a list of tuples of path:property_name:offset, each of
 * which refers to a property that is phandle to a node in the base
 * devicetree.
 */
static int update_usages_of_a_phandle_reference(struct device_node *overlay,
						struct property *prop_fixup,
						phandle phandle)
{
	struct device_node *refnode;
	struct property *prop;
	char *value, *cur, *end, *node_path, *prop_name, *s;
	int offset, len;
	int err = 0;

	pr_debug("resolve references to %s to phandle 0x%x\n",
		 prop_fixup->name, phandle);

	value = kmemdup(prop_fixup->value, prop_fixup->length, GFP_KERNEL);
	if (!value)
		return -ENOMEM;

	end = value + prop_fixup->length;
	for (cur = value; cur < end; cur += len + 1) {
		len = strlen(cur);

		node_path = cur;
		s = strchr(cur, ':');
		if (!s) {
			err = -EINVAL;
			goto err_fail;
		}
		*s++ = '\0';

		prop_name = s;
		s = strchr(s, ':');
		if (!s) {
			err = -EINVAL;
			goto err_fail;
		}
		*s++ = '\0';

		err = kstrtoint(s, 10, &offset);
		if (err)
			goto err_fail;

		refnode = of_find_node_by_path_from(overlay, node_path);
		if (!refnode)
			continue;

		prop = of_find_property(refnode, prop_name, NULL);
		if (!prop) {
			err = -ENOENT;
			goto err_fail;
		}

		if (offset < 0 || offset + sizeof(__be32) > prop->length) {
			err = -EINVAL;
			goto err_fail;
		}

		*(__be32 *)(prop->value + offset) = cpu_to_be32(phandle);
	}

err_fail:
	kfree(value);

	if (err)
		pr_debug("failed to resolve references to %s\n",
			 prop_fixup->name);

	return err;
}

/*
 * Adjust the local phandle references by the given phandle delta.
 *
 * Subtree @local_fixups, which is overlay node __local_fixups__,
 * mirrors the fragment node structure at the root of the overlay.
 *
 * For each property in the fragments that contains a phandle reference,
 * @local_fixups has a property of the same name that contains a list
 * of offsets of the phandle reference(s) within the respective property
 * value(s).  The values at these offsets will be fixed up.
 */
static int adjust_local_phandle_references(struct device_node *local_fixups,
		struct device_node *overlay, int phandle_delta)
{
	struct device_node *child, *overlay_child;
	struct property *prop_fix, *prop;
	int err, i, count;
	unsigned int off;

	if (!local_fixups)
		return 0;

	list_for_each_entry(prop_fix, &local_fixups->properties, list) {
		if (!of_prop_cmp(prop_fix->name, "name") ||
		    !of_prop_cmp(prop_fix->name, "phandle") ||
		    !of_prop_cmp(prop_fix->name, "linux,phandle"))
			continue;

		if ((prop_fix->length % sizeof(__be32)) != 0 ||
		    prop_fix->length == 0)
			return -EINVAL;
		count = prop_fix->length / sizeof(__be32);

		prop = of_find_property(overlay, prop_fix->name, NULL);
		if (!prop)
			return -EINVAL;

		for (i = 0; i < count; i++) {
			off = be32_to_cpu(((__be32 *)prop_fix->value)[i]);
			if ((off + sizeof(__be32)) > prop->length)
				return -EINVAL;

			be32_add_cpu(prop->value + off, phandle_delta);
		}
	}

	for_each_child_of_node(local_fixups, child) {
		for_each_child_of_node(overlay, overlay_child)
			if (!of_node_cmp(child->name, overlay_child->name))
				break;
		if (!overlay_child)
			return -EINVAL;

		err = adjust_local_phandle_references(child, overlay_child,
				phandle_delta);
		if (err)
			return err;
	}

	return 0;
}

/**
 * of_resolve_phandles - Resolve phandles in overlay based on root
 *
 * Rename phandles in overlay to avoid conflicts with the base devicetree and
 * replace all phandles in the overlay with their renamed versions. Resolve
 * phandles referring to nodes in the base devicetree with the phandle from
 * the base devicetree.
 *
 * Returns a new device_node with resolved phandles which must be deleted by
 * the caller of this function.
 */
struct device_node *of_resolve_phandles(struct device_node *root,
					const struct device_node *overlay)
{
	struct device_node *result;
	struct device_node *local_fixups;
	struct device_node *refnode;
	struct device_node *symbols;
	struct device_node *overlay_fixups;
	struct property *prop;
	const char *refpath;
	phandle delta;
	int err;

	result = of_copy_node(NULL, overlay);
	if (!result)
		return NULL;

	delta = of_get_tree_max_phandle(root) + 1;

	/*
	 * Rename the phandles in the devicetree overlay to prevent conflicts
	 * with the phandles in the base devicetree.
	 */
	adjust_overlay_phandles(result, delta);

	/*
	 * __local_fixups__ contains all locations in the overlay that refer
	 * to a phandle defined in the overlay. We must update the references,
	 * because we just adjusted the definitions.
	 */
	local_fixups = of_find_node_by_name(result, "__local_fixups__");
	err = adjust_local_phandle_references(local_fixups, result, delta);
	if (err) {
		pr_err("failed to fix phandles in overlay\n");
		goto err;
	}

	/*
	 * __fixups__ contains all locations in the overlay that refer to a
	 * phandle that is not defined in the overlay and should be defined in
	 * the base device tree. We must update the references, because they
	 * are otherwise undefined.
	 */
	overlay_fixups = of_find_node_by_name(result, "__fixups__");
	if (!overlay_fixups) {
		pr_debug("overlay does not contain phandles to base devicetree\n");
		goto out;
	}

	symbols = of_find_node_by_path_from(root, "/__symbols__");
	if (!symbols) {
		pr_err("__symbols__ missing from base devicetree\n");
		goto err;
	}

	list_for_each_entry(prop, &overlay_fixups->properties, list) {
		if (!of_prop_cmp(prop->name, "name"))
			continue;

		err = of_property_read_string(symbols, prop->name, &refpath);
		if (err) {
			pr_err("cannot find node %s in base devicetree\n",
			       prop->name);
			goto err;
		}

		refnode = of_find_node_by_path_from(root, refpath);
		if (!refnode) {
			pr_err("cannot find path %s in base devicetree\n",
			       refpath);
			err = -EINVAL;
			goto err;
		}

		err = update_usages_of_a_phandle_reference(result, prop,
							   refnode->phandle);
		if (err) {
			pr_err("failed to update phandles for %s in overlay",
			       prop->name);
			goto err;
		}
	}

out:
	return result;
err:
	of_delete_node(result);

	return NULL;

}
