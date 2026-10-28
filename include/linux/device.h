/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LINUX_DRIVER_H_
#define LINUX_DRIVER_H_

#include <driver.h>
#include <linux/slab.h>
#include <linux/bug.h>
#include <linux/bitops.h>
#include <linux/overflow.h>
#include <stdarg.h>
#include <mmu.h>

#define device_driver driver

/*
 * Managed memory is DMA-safe and can't fail to allocate. It's freed when
 * the device is unbound, unregistered or fails to probe.
 */
void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp);
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp);
void devm_kfree(struct device *dev, const void *mem);
void *devm_kmemdup(struct device *dev, const void *src, size_t len, gfp_t gfp);
char *devm_kstrdup(struct device *dev, const char *s, gfp_t gfp);
const char *devm_kstrdup_const(struct device *dev, const char *s, gfp_t gfp);
char *devm_kvasprintf(struct device *dev, gfp_t gfp, const char *fmt,
		      va_list ap);
char *devm_kasprintf(struct device *dev, gfp_t gfp, const char *fmt, ...)
	__printf(3, 4);

static inline void *devm_kmalloc_array(struct device *dev, size_t n,
				       size_t size, gfp_t gfp)
{
	return devm_kmalloc(dev, size_mul(n, size), gfp);
}

static inline void *devm_kcalloc(struct device *dev, size_t n, size_t size,
				 gfp_t gfp)
{
	return devm_kzalloc(dev, size_mul(n, size), gfp);
}

static inline unsigned long *devm_bitmap_zalloc(struct device *dev,
						unsigned int nbits, gfp_t gfp)
{
	return devm_kcalloc(dev, BITS_TO_LONGS(nbits), sizeof(unsigned long),
			    gfp);
}

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
	return dev_request_mem_region(dev, resource);
}

static inline void __iomem *devm_ioremap(struct device *dev,
					 resource_size_t start,
					 resource_size_t size)
{
	if (start)
		remap_range((void *)start, size, MAP_UNCACHED);

	return IOMEM(start);
}

/**
 * dev_set_drvdata - set driver private data for device
 * @dev: device instance
 * @data: driver-specific data
 *
 * Returns private driver data or NULL if none was set.
 *
 * NOTE: This does _not_ return the match data associated with
 * the match. For that use device_get_match_data instead.
 */
static inline void dev_set_drvdata(struct device *dev, void *data)
{
	dev->driver_data = data;
}

/**
 * dev_get_drvdata - get driver match data associated for device
 * @dev: device instance
 * @data: driver-specific data
 *
 * Set some driver and device specific data for later retrieval
 * by dev_get_drvdata.
 *
 * NOTE: This does _not_ return the match data associated with
 * the match. For that use device_get_match_data instead.
 */
static inline void *dev_get_drvdata(const struct device *dev)
{
	return dev->driver_data;
}

#endif
