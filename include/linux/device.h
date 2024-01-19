/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LINUX_DRIVER_H_
#define LINUX_DRIVER_H_

#include <driver.h>
#include <linux/slab.h>
#include <linux/bug.h>
#include <mmu.h>

#define device_driver driver

#define __devm_wrapper(fn, dev, ...) ({ BUG_ON(!dev); fn(__VA_ARGS__); })

#define devm_kmalloc(...)		__devm_wrapper(kmalloc, __VA_ARGS__)
#define devm_krealloc(...)		__devm_wrapper(krealloc, __VA_ARGS__)
#define devm_kvasprintf(...)		__devm_wrapper(kvasprintf, __VA_ARGS__)
#define devm_kasprintf(...)		__devm_wrapper(kasprintf, __VA_ARGS__)
#define devm_kzalloc(...)		__devm_wrapper(kzalloc, __VA_ARGS__)
#define devm_kmalloc_array(...)		__devm_wrapper(kmalloc_array, __VA_ARGS__)
#define devm_kcalloc(...)		__devm_wrapper(kcalloc, __VA_ARGS__)
#define devm_krealloc_array(...)	__devm_wrapper(krealloc_array, __VA_ARGS__)
#define devm_kfree(...)			__devm_wrapper(kfree, __VA_ARGS__)
#define devm_kstrdup(...)		__devm_wrapper(kstrdup, __VA_ARGS__)
#define devm_kstrdup_const(...)		__devm_wrapper(kstrdup_const, __VA_ARGS__)
#define devm_kmemdup(...)		__devm_wrapper(kmemdup, __VA_ARGS__)
#define devm_bitmap_zalloc(dev, nbits, gfp)	\
	__devm_wrapper(bitmap_zalloc, dev, nbits)

#define device_register register_device
#define device_unregister unregister_device

#define driver_register register_driver
#define driver_unregister unregister_driver


static inline void __iomem *dev_platform_ioremap_resource(struct device *dev,
							  int resource)
{
	/*
	 * barebox maps everything outside the RAM banks suitably for MMIO,
	 * so we don't need to do anything besides requesting the regions
	 * and can leave the memory attributes unchanged.
	 */
	return dev_request_mem_region_err_null(dev, resource);
}

static inline void __iomem *devm_ioremap(struct device *dev,
					 resource_size_t start,
					 resource_size_t size)
{
	if (start)
		remap_range((void *)start, size, MAP_UNCACHED);

	return IOMEM(start);
}

static inline int bus_for_each_dev(const struct bus_type *bus, struct device *start, void *data,
				   int (*fn)(struct device *dev, void *data))
{
	struct device *dev;
	int ret;

	bus_for_each_device(bus, dev) {
		if (start) {
			if (dev == start)
				start = NULL;
			continue;
		}

		ret = fn(dev, data);
		if (ret)
			return ret;
	}

	return 0;
}

#endif
