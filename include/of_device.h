/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __OF_DEVICE_H
#define __OF_DEVICE_H

#include <driver.h>
#include <of.h>


#ifdef CONFIG_OFTREE
extern const struct of_device_id *of_match_device(
	const struct of_device_id *matches, const struct device *dev);

/**
 * of_driver_match_device - Tell if a driver's of_match_table matches a device.
 * @drv: the device_driver structure to test
 * @dev: the device structure to match against
 */
static inline int of_driver_match_device(struct device *dev,
					 const struct driver *drv)
{
	return of_match_device(drv->of_compatible, dev) != NULL;
}

extern const void *of_device_get_match_data(const struct device *dev);
extern const char *of_device_get_match_compatible(const struct device *dev);
void of_device_make_bus_id(struct device *dev);

#else /* CONFIG_OFTREE */

static inline int of_driver_match_device(struct device *dev,
					 const struct device *drv)
{
	return 0;
}

static inline const void *of_device_get_match_data(const struct device *dev)
{
	return NULL;
}

static inline const char *of_device_get_match_compatible(const struct device *dev)
{
	return NULL;
}

static inline const struct of_device_id *__of_match_device(
		const struct of_device_id *matches, const struct device *dev)
{
	return NULL;
}
#define of_match_device(matches, dev)	\
	__of_match_device(matches, (dev))

static inline void of_device_make_bus_id(struct device *dev) {}

#endif /* CONFIG_OFTREE */

#endif /* _LINUX_OF_DEVICE_H */
