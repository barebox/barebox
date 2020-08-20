#include <common.h>
#include <of.h>
#include <of_device.h>

/**
 * of_match_device - Tell if a struct device matches an of_device_id list
 * @ids: array of of device match structures to search in
 * @dev: the of device structure to match against
 *
 * Used by a driver to check whether an platform_device present in the
 * system is in its list of supported devices.
 */
const struct of_device_id *of_match_device(const struct of_device_id *matches,
					   const struct device_d *dev)
{
	if ((!matches) || (!dev->device_node))
		return NULL;

	return of_match_node(matches, dev->device_node);
}
EXPORT_SYMBOL(of_match_device);

const void *of_device_get_match_data(const struct device_d *dev)
{
	const struct of_device_id *match;

	match = of_match_device(dev->driver->of_compatible, dev);
	if (!match)
		return NULL;

	return match->data;
}
EXPORT_SYMBOL(of_device_get_match_data);

const char *of_device_get_match_compatible(const struct device_d *dev)
{
	const struct of_device_id *match;

	match = of_match_device(dev->driver->of_compatible, dev);
	if (!match)
		return NULL;

	return match->compatible;
}
EXPORT_SYMBOL(of_device_get_match_compatible);
