#ifndef __OF_DEVICE_H
#define __OF_DEVICE_H

#include <driver.h>
#include <of.h>


#ifdef CONFIG_OFTREE
extern const struct of_device_id *of_match_device(
	const struct of_device_id *matches, const struct device_d *dev);

/**
 * of_driver_match_device - Tell if a driver's of_match_table matches a device.
 * @drv: the device_driver structure to test
 * @dev: the device structure to match against
 */
static inline int of_driver_match_device(struct device_d *dev,
					 const struct driver_d *drv)
{
	return of_match_device(drv->of_compatible, dev) != NULL;
}

extern const void *of_device_get_match_data(const struct device_d *dev);

#else /* CONFIG_OF */

static inline int of_driver_match_device(struct device_d *dev,
					 const struct device_d *drv)
{
	return 0;
}

static inline const void *of_device_get_match_data(const struct device_d *dev)
{
	return NULL;
}

static inline const struct of_device_id *__of_match_device(
		const struct of_device_id *matches, const struct device_d *dev)
{
	return NULL;
}
#define of_match_device(matches, dev)	\
	__of_match_device(matches, (dev))

#endif /* CONFIG_OF */

#endif /* _LINUX_OF_DEVICE_H */
