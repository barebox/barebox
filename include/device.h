/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <linux/types.h>
#include <linux/list.h>
#include <init.h>
#include <bobject.h>

enum dev_dma_coherence {
	DEV_DMA_COHERENCE_DEFAULT = 0,
	DEV_DMA_COHERENT,
	DEV_DMA_NON_COHERENT,
};

struct device;
struct bus_type;
struct resource;
struct driver;
struct generic_pm_domain;
struct platform_device_id;
struct of_device_id;

/**
 * struct device - The basic device structure
 * @bobject: base class barebox object
 * @name: This member is used to match with a driver. This is a
 *        descriptive name and could be MPC5XXX_ether or imx_serial.
 *        Unless absolutely necessary, should not be modified
 *        directly and dev_set_name() should be used instead.
 * @unique_name: Device's unique name as obtained by calling dev_id().
 *               Internal field, do not access it directly.
 * @id: Used to uniquely identify a device in the system. The id
 *      will show up under /dev/ as the device's name. Usually this
 *      is something like eth0 or nor0.
 * @dma_coherent: DMA coherence setting for this device.
 * @resource: Array of resources for this device.
 * @num_resources: Number of resources in the resource array.
 * @platform_data: Board specific information about this device.
 * @priv: Device class specific data storage.
 * @driver_data: Alternative name for @priv
 * @type_data: In case this device is a specific device, this pointer
 *             points to the type specific device, i.e. eth_device
 * @driver: The driver for this device.
 * @list: The list of all devices.
 * @bus_list: Our bus.
 * @children: Our children.
 * @sibling: Our siblings.
 * @active: The list of all devices which have a driver
 * @parent: The device's "parent" device, the device to which it is attached.
 *          If parent is NULL, the device is a top-level device.
 * @pm_domain: Attached power domain.
 * @bus: Type of bus device is on.
 * @parameters: The parameters for this device. This is used to carry information
 *              of board specific data from the board code to the device driver
 * @dma_mask: DMA mask.
 * @dma_offset: DMA offset.
 * @detect: For devices which take longer to probe, this is called when the driver
 *          should actually detect client devices.
 * @rescan: Callback to rescan the device.
 * @deferred_probe_reason: If a driver probe is deferred, this stores the last error.
 */
struct device {
	union {
		struct bobject bobject;
		char *name;
	};

	char *unique_name;
	int id;

	enum dev_dma_coherence dma_coherent;

	struct resource *resource;
	int num_resources;

	void *platform_data;

	union {
		void *priv;
		void *driver_data;
	};

	void *type_data;

	struct driver *driver;

	struct list_head list;
	struct list_head bus_list;
	struct list_head children;
	struct list_head sibling;
	struct list_head active;

	struct device *parent;

	struct generic_pm_domain *pm_domain;

	struct bus_type *bus;

	struct list_head parameters;

	struct list_head cdevs;

	struct list_head class_list;

	const struct platform_device_id *id_entry;
	union {
		struct device_node *device_node;
		struct device_node *of_node;
	};
	const struct of_device_id *of_id_entry;

	u64 dma_mask;

	unsigned long dma_offset;

#ifdef CONFIG_CMD_DEVINFO
	struct list_head info_list;
#endif

	int (*detect)(struct device *);
	void (*rescan)(struct device *);

	char *deferred_probe_reason;
};

struct class {
	const char *name;
	struct list_head devices;
	struct list_head list;
};

void class_register(struct class *class);

#define DEFINE_DEV_CLASS(_name, _classname)			\
	struct class _name = {					\
		.name = _classname,				\
		.devices = LIST_HEAD_INIT(_name.devices),	\
	};							\
	static int register_##_name(void) {			\
		class_register(&_name);				\
		return 0;					\
	}							\
	pure_initcall(register_##_name);

int class_add_device(struct class *class, struct device *dev);

int class_register_device(struct class *class, struct device *class_dev,
			  const char *name);

extern struct list_head class_list;
#define class_for_each_device(class, dev) list_for_each_entry((dev), &(class)->devices, class_list)
#define class_first_device(class) \
	list_first_entry_or_null(&(class)->devices, struct device, class_list)

#define class_for_each_container_of_device(class, obj, member) \
	list_for_each_entry((obj), &(class)->devices, member.class_list)

#define class_for_each(class) list_for_each_entry((class), &class_list, list)

#define dev_for_each_param(dev, param) \
	list_for_each_entry((param), &(dev)->parameters, list)
#define dev_for_each_param_safe(dev, param, tmp) \
	list_for_each_entry_safe((param), (tmp), &(dev)->parameters, list)

struct device_alias {
	struct device *dev;
	struct list_head list;
	char name[];
};

static inline struct device_node *dev_of_node(struct device *dev)
{
	return IS_ENABLED(CONFIG_OFDEVICE) ? dev->of_node : NULL;
}

/* dynamically assign the next free id */
#define DEVICE_ID_DYNAMIC	-2
/* do not use an id (only one device available) */
#define DEVICE_ID_SINGLE	-1

static inline const char *dev_id(const struct device *dev)
{
	if (!dev)
		return NULL;
	return (dev->id != DEVICE_ID_SINGLE) ? dev->unique_name : dev->name;
}

static inline const char *dev_name(const struct device *dev)
{
	if (!dev)
		return NULL;
	return dev_id(dev) ?: dev->name;
}

#define dev_set_name bobject_set_name

static inline bool dev_is_dma_coherent(struct device *dev)
{
	if (dev) {
		switch (dev->dma_coherent) {
		case DEV_DMA_NON_COHERENT:
			return false;
		case DEV_DMA_COHERENT:
			return true;
		case DEV_DMA_COHERENCE_DEFAULT:
			break;
		}
	}

	return IS_ENABLED(CONFIG_ARCH_DMA_DEFAULT_COHERENT);
}

#ifdef CONFIG_CMD_DEVINFO
void devinfo_add(struct device *dev, void (*info)(struct device *));
void devinfo_del(struct device *dev, void (*info)(struct device *));
void devinfo(struct device *dev);
#else
static inline void devinfo_add(struct device *dev, void (*info)(struct device *)) {}
static inline void devinfo_del(struct device *dev, void (*info)(struct device *)) {}
static inline void devinfo(struct device *dev) {}
#endif

#endif
