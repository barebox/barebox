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

/**
 * __devm_alloc_objs - Device-managed allocation of objects of a given type
 * @DEVM_ALLOC: which device-managed size-based allocator to use.
 * @DEV: device to scope the allocation lifetime to.
 * @GFP: GFP flags for the allocation.
 * @TYPE: type to allocate space for.
 * @COUNT: how many @TYPE objects to allocate.
 *
 * Device-managed counterpart of __alloc_objs().
 * @DEVM_ALLOC takes (@DEV, size, gfp).
 *
 * Returns: Newly allocated pointer to (first) @TYPE of @COUNT-many allocated
 * @TYPE objects.
 */
#define __devm_alloc_objs(DEVM_ALLOC, DEV, GFP, TYPE, COUNT)		\
({									\
	const size_t __obj_size = size_mul(sizeof(TYPE), COUNT);	\
	(TYPE *)DEVM_ALLOC(DEV, __obj_size, GFP);			\
})

/**
 * devm_kmalloc_obj - Device-managed allocation of a single typed object
 * @DEV: Device to scope the allocation lifetime to.
 * @VAR_OR_TYPE: Variable or type to allocate.
 * @GFP: Optional GFP flags (defaults to %GFP_KERNEL).
 *
 * Device-managed counterpart of kmalloc_obj(): the allocation is sized from
 * @VAR_OR_TYPE and returned as a pointer to that type, not void *.
 *
 * Returns: newly allocated pointer to a @VAR_OR_TYPE.
 */
#define devm_kmalloc_obj(DEV, VAR_OR_TYPE, ...)				\
	__devm_alloc_objs(devm_kmalloc, DEV, default_gfp(__VA_ARGS__),	\
			  typeof(VAR_OR_TYPE), 1)

/**
 * devm_kmalloc_objs - Device-managed allocation of an array of a typed object
 * @DEV: Device to scope the allocation lifetime to.
 * @VAR_OR_TYPE: Variable or type to allocate an array of.
 * @COUNT: How many elements in the array.
 * @GFP: Optional GFP flags (defaults to %GFP_KERNEL).
 *
 * Returns: newly allocated pointer to an array of @VAR_OR_TYPE.
 */
#define devm_kmalloc_objs(DEV, VAR_OR_TYPE, COUNT, ...) \
	__devm_alloc_objs(devm_kmalloc, DEV, default_gfp(__VA_ARGS__), \
			  typeof(VAR_OR_TYPE), COUNT)

/* All devm_kzalloc aliases for devm_kmalloc_(obj|objs). */
#define devm_kzalloc_obj(DEV, P, ...) \
	__devm_alloc_objs(devm_kzalloc, DEV, default_gfp(__VA_ARGS__), typeof(P), 1)
#define devm_kzalloc_objs(DEV, P, COUNT, ...) \
	__devm_alloc_objs(devm_kzalloc, DEV, default_gfp(__VA_ARGS__), \
			  typeof(P), COUNT)

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

/**
 * devm_add_action() - add a custom action to the managed resources
 * @dev: device that owns the action
 * @action: function to call when the device is unbound, unregistered
 *          or shut down
 * @data: argument to pass to @action
 *
 * Actions run in reverse order of registration, interleaved with the
 * freeing of managed memory.
 *
 * Return: 0 on success, -ENOMEM if the action could not be registered.
 */
int devm_add_action(struct device *dev, void (*action)(void *), void *data);

/**
 * devm_add_action_or_reset() - add a custom action, running it on failure
 * @dev: device that owns the action
 * @action: function to call when the device is unbound, unregistered
 *          or shut down
 * @data: argument to pass to @action
 *
 * Like devm_add_action(), but @action is called right away if it could
 * not be registered.
 *
 * Return: 0 on success, -ENOMEM if the action could not be registered.
 */
static inline int devm_add_action_or_reset(struct device *dev,
					   void (*action)(void *), void *data)
{
	int ret;

	ret = devm_add_action(dev, action, data);
	if (ret)
		action(data);

	return ret;
}

#define device_register register_device
#define device_unregister unregister_device

#define driver_register register_driver
#define driver_unregister unregister_driver


void __iomem *devm_platform_ioremap_resource(struct device *dev, int num);

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
