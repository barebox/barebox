// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <of.h>
#include <of_device.h>
#include <of_address.h>

/**
 * of_match_device - Tell if a struct device matches an of_device_id list
 * @ids: array of of device match structures to search in
 * @dev: the of device structure to match against
 *
 * Used by a driver to check whether an platform_device present in the
 * system is in its list of supported devices.
 */
const struct of_device_id *of_match_device(const struct of_device_id *matches,
					   const struct device *dev)
{
	if ((!matches) || (!dev->of_node))
		return NULL;

	return of_match_node(matches, dev->of_node);
}
EXPORT_SYMBOL(of_match_device);

const void *of_device_get_match_data(const struct device *dev)
{
	const struct of_device_id *match;

	match = of_match_device(dev->driver->of_compatible, dev);
	if (!match)
		return NULL;

	return match->data;
}
EXPORT_SYMBOL(of_device_get_match_data);

const char *of_device_get_match_compatible(const struct device *dev)
{
	const struct of_device_id *match;

	match = of_match_device(dev->driver->of_compatible, dev);
	if (!match)
		return NULL;

	return match->compatible;
}
EXPORT_SYMBOL(of_device_get_match_compatible);

/**
 * of_device_make_bus_id - Use the device node data to assign a unique name
 * @dev: pointer to device structure that is linked to a device tree node
 *
 * This routine will first try using the translated bus address to
 * derive a unique name. If it cannot, then it will prepend names from
 * parent nodes until a unique name can be derived.
 */
void of_device_make_bus_id(struct device *dev)
{
	struct device_node *node = dev->of_node;
	const __be32 *reg;
	u64 addr;

	/* Construct the name, using parent nodes if necessary to ensure uniqueness */
	while (node->parent) {
		/*
		 * If the address can be translated, then that is as much
		 * uniqueness as we need. Make it the first component and return
		 */
		reg = of_get_property(node, "reg", NULL);
		if (reg && (addr = of_translate_address(node, reg)) != OF_BAD_ADDR) {
			dev_set_name(dev, dev->name ? "%llx.%s:%s" : "%llx.%s.of",
				     (unsigned long long)addr, node->name,
				     dev->name);
			return;
		}

		/* format arguments only used if dev_name() resolves to NULL */
		dev_set_name(dev, dev->name ? "%s:%s" : "%s.of",
			     kbasename(node->full_name), dev->name);
		node = node->parent;
	}
}
EXPORT_SYMBOL_GPL(of_device_make_bus_id);
