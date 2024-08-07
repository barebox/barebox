/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <linux/types.h>
#include <linux/list.h>
#include <init.h>

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

/** @brief Describes a particular device present in the system */
struct device {
	/*! This member (and 'type' described below) is used to match
	 * with a driver. This is a descriptive name and could be
	 * MPC5XXX_ether or imx_serial. Unless absolutely necessary,
	 * should not be modified directly and dev_set_name() should
	 * be used instead.
	 */
	char *name;

	/*! This member is used to store device's unique name as
	 *  obtained by calling dev_id(). Internal field, do not
	 *  access it directly.
	  */
	char *unique_name;
	/*! The id is used to uniquely identify a device in the system. The id
	 * will show up under /dev/ as the device's name. Usually this is
	 * something like eth0 or nor0. */
	int id;

	enum dev_dma_coherence dma_coherent;

	struct resource *resource;
	int num_resources;

	void *platform_data; /*! board specific information about this device */

	/*! Devices of a particular class normaly need to store more
	 * information than struct device holds.
	 */
	void *priv;
	void *type_data;     /*! In case this device is a specific device, this pointer
			      * points to the type specific device, i.e. eth_device
			      */
	struct driver *driver; /*! The driver for this device */

	struct list_head list;     /* The list of all devices */
	struct list_head bus_list; /* our bus            */
	struct list_head children; /* our children            */
	struct list_head sibling;
	struct list_head active;   /* The list of all devices which have a driver */

	struct device *parent;   /* our parent, NULL if not present */

	struct generic_pm_domain *pm_domain;	/* attached power domain */

	struct bus_type *bus;

	/*! The parameters for this device. This is used to carry information
	 * of board specific data from the board code to the device driver. */
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

	void    (*info) (struct device *);
	/*
	 * For devices which take longer to probe this is called
	 * when the driver should actually detect client devices
	 */
	int     (*detect) (struct device *);
	void	(*rescan) (struct device *);

	/*
	 * if a driver probe is deferred, this stores the last error
	 */
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
void class_remove_device(struct class *class, struct device *dev);

int class_register_device(struct class *class, struct device *class_dev,
			  const char *name);
void class_unregister_device(struct device *class_dev);

extern struct list_head class_list;
#define class_for_each_device(class, dev) list_for_each_entry((dev), &(class)->devices, class_list)
#define class_for_each(class) list_for_each_entry((class), &class_list, list)

struct device_alias {
	struct device *dev;
	struct list_head list;
	char name[];
};

static inline struct device_node *dev_of_node(struct device *dev)
{
	return IS_ENABLED(CONFIG_OFDEVICE) ? dev->of_node : NULL;
}

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

#endif
