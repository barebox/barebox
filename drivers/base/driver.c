// SPDX-License-Identifier: GPL-2.0-only
/*
 * driver.c - barebox driver model
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

/**
 * @file
 * @brief barebox's driver model, and devinfo command
 */

#define dev_err_probe dev_err_probe

#include <common.h>
#include <command.h>
#include <deep-probe.h>
#include <driver.h>
#include <malloc.h>
#include <console.h>
#include <linux/ctype.h>
#include <errno.h>
#include <init.h>
#include <fs.h>
#include <of.h>
#include <linux/list.h>
#include <linux/overflow.h>
#include <linux/err.h>
#include <complete.h>
#include <pinctrl.h>
#include <featctrl.h>
#include <linux/clk/clk-conf.h>

#ifdef CONFIG_DEBUG_PROBES
#define pr_report_probe		pr_info
#else
#define pr_report_probe		pr_debug
#endif

LIST_HEAD(device_list);
EXPORT_SYMBOL(device_list);

LIST_HEAD(driver_list);
EXPORT_SYMBOL(driver_list);

LIST_HEAD(active_device_list);
EXPORT_SYMBOL(active_device_list);
static LIST_HEAD(deferred);

static LIST_HEAD(device_alias_list);

struct device *find_device(const char *str)
{
	struct device *dev;
	struct device_node *np;

	dev = get_device_by_name(str);
	if (dev)
		return dev;

	np = of_find_node_by_path_or_alias(NULL, str);
	if (np)
		return of_find_device_by_node(np);

	return NULL;
}

struct device *get_device_by_name(const char *name)
{
	struct device *dev;
	struct device_alias *alias;

	for_each_device(dev) {
		if(!strcmp(dev_name(dev), name))
			return dev;
	}

	list_for_each_entry(alias, &device_alias_list, list) {
		if(!strcmp(alias->name, name))
			return alias->dev;
	}

	return NULL;
}

static struct device *get_device_by_name_id(const char *name, int id)
{
	struct device *dev;

	for_each_device(dev) {
		if(!strcmp(dev->name, name) && id == dev->id)
			return dev;
	}

	return NULL;
}

int get_free_deviceid(const char *name_template)
{
	int i = 0;

	while (1) {
		if (!get_device_by_name_id(name_template, i))
			return i;
		i++;
	};
}

static void dev_report_permanent_probe_deferral(struct device *dev)
{
	if (dev->deferred_probe_reason)
		dev_err(dev, "probe permanently deferred (%s)\n",
			dev->deferred_probe_reason);
	else
		dev_err(dev, "probe permanently deferred\n");
}

int device_probe(struct device *dev)
{
	static int depth = 0;
	int ret;

	ret = of_feature_controller_check(dev->of_node);
	if (ret < 0)
		return ret;
	if (ret == FEATCTRL_GATED) {
		dev_dbg(dev, "feature gated, skipping probe\n");
		return -ENODEV;
	}

	depth++;

	pr_report_probe("%*sprobe-> %s\n", depth * 4, "", dev_name(dev));

	pinctrl_select_state_default(dev);
	of_clk_set_defaults(dev->of_node, false);

	list_add(&dev->active, &active_device_list);

	if (dev->bus->probe)
		ret = dev->bus->probe(dev);
	else if (dev->driver->probe)
		ret = dev->driver->probe(dev);
	else
		ret = 0;

	depth--;

	switch (ret) {
	case 0:
		return 0;
	case -EPROBE_DEFER:
		/*
		 * -EPROBE_DEFER should never appear on a deep-probe machine so
		 * inform the user immediately.
		 */
		if (deep_probe_is_supported()) {
			dev_report_permanent_probe_deferral(dev);
			break;
		}

		list_move(&dev->active, &deferred);

		dev_dbg(dev, "probe deferred\n");
		return -EPROBE_DEFER;
	case -ENODEV:
	case -ENXIO:
		dev_dbg(dev, "probe failed: %pe\n", ERR_PTR(ret));
		break;
	default:
		dev_err(dev, "probe failed: %pe\n", ERR_PTR(ret));
		break;
	}

	list_del_init(&dev->active);

	return ret;
}

int device_detect(struct device *dev)
{
	if (!dev->detect)
		return -ENOSYS;
	return dev->detect(dev);
}

int device_detect_by_name(const char *__devname)
{
	char *devname = xstrdup(__devname);
	char *str = devname;
	struct device *dev;
	int ret = -ENODEV;

	while (1) {
		strsep(&str, ".");

		dev = get_device_by_name(devname);
		if (dev)
			ret = device_detect(dev);

		if (!str)
			break;
		else
			*(str - 1) = '.';
	}

	free(devname);

	return ret;
}

void device_detect_all(void)
{
	struct device *dev;

	for_each_device(dev)
		device_detect(dev);
}

static int match(struct driver *drv, struct device *dev)
{
	int ret;

	if (dev->driver)
		return -1;

	dev->driver = drv;

	if (!driver_match_device(drv, dev))
		goto err_out;
	ret = device_probe(dev);
	if (ret)
		goto err_out;

	return 0;
err_out:
	dev->driver = NULL;
	return -1;
}

int register_device(struct device *new_device)
{
	struct driver *drv;

	if (new_device->id == DEVICE_ID_DYNAMIC) {
		new_device->id = get_free_deviceid(new_device->name);
	} else {
		if (get_device_by_name_id(new_device->name, new_device->id)) {
			pr_err("register_device: already registered %s\n",
				dev_name(new_device));
			return -EINVAL;
		}
	}

	if (new_device->id != DEVICE_ID_SINGLE)
		new_device->unique_name = basprintf(FORMAT_DRIVER_NAME_ID,
						    new_device->name,
						    new_device->id);

	debug ("register_device: %s\n", dev_name(new_device));

	list_add_tail(&new_device->list, &device_list);
	INIT_LIST_HEAD(&new_device->children);
	INIT_LIST_HEAD(&new_device->cdevs);
	INIT_LIST_HEAD(&new_device->active);
	INIT_LIST_HEAD(&new_device->bus_list);
	INIT_LIST_HEAD(&new_device->class_list);

	bobject_init(&new_device->bobject);

	of_pinctrl_register_consumer(new_device, new_device->device_node);

	if (new_device->bus) {
		if (!new_device->parent)
			new_device->parent = &new_device->bus->dev;

		list_add_tail(&new_device->bus_list, &new_device->bus->device_list);

		bus_for_each_driver(new_device->bus, drv) {
			if (!match(drv, new_device))
				break;
		}
	}

	if (new_device->parent)
		list_add_tail(&new_device->sibling, &new_device->parent->children);

	return 0;
}
EXPORT_SYMBOL(register_device);

int unregister_device(struct device *old_dev)
{
	struct device_alias *alias, *at;
	struct cdev *cdev, *ct;
	struct device *child, *dt;
	struct device_node *np;

	dev_dbg(old_dev, "unregister\n");

	bobject_del(&old_dev->bobject);

	of_pinctrl_unregister_consumer(old_dev);

	if (old_dev->driver)
		device_remove(old_dev);

	list_for_each_entry_safe(alias, at, &device_alias_list, list) {
		if(alias->dev == old_dev)
			list_del(&alias->list);
	}

	list_for_each_entry_safe(child, dt, &old_dev->children, sibling) {
		dev_dbg(old_dev, "unregister child %s\n", dev_name(child));
		unregister_device(child);
	}

	list_for_each_entry_safe(cdev, ct, &old_dev->cdevs, devices_list) {
		if (cdev_is_partition(cdev)) {
			dev_dbg(old_dev, "unregister part %s\n", cdev->name);
			cdevfs_del_partition(cdev);
		}
	}

	list_del(&old_dev->list);
	list_del(&old_dev->bus_list);
	list_del(&old_dev->class_list);
	list_del(&old_dev->active);

	/* remove device from parents child list */
	if (old_dev->parent)
		list_del(&old_dev->sibling);

	np = dev_of_node(old_dev);
	if (np && np->dev == old_dev)
		np->dev = NULL;

	return 0;
}
EXPORT_SYMBOL(unregister_device);

/**
 * free_device_res - free dynamically allocated device members
 * @dev: The device
 *
 * This frees dynamically allocated resources allocated during device
 * lifetime, but not the device itself.
 */
void free_device_res(struct device *dev)
{
	free(dev->name);
	dev->name = NULL;
	free(dev->unique_name);
	dev->unique_name = NULL;
	free(dev->deferred_probe_reason);
}
EXPORT_SYMBOL(free_device_res);

/**
 * free_device - free a device
 * @dev: The device
 *
 * This frees dynamically allocated resources allocated during device
 * lifetime and finally the device itself.
 */
void free_device(struct device *dev)
{
	free_device_res(dev);
	free(dev);
}
EXPORT_SYMBOL(free_device);

/*
 * Loop over list of deferred devices as long as at least one
 * device is successfully probed. Devices that again request
 * deferral are re-added to deferred list in device_probe().
 * For devices finally left in deferred list -EPROBE_DEFER
 * becomes a fatal error.
 */
static int device_probe_deferred(void)
{
	struct device *dev, *tmp;
	struct driver *drv;
	bool success;

	do {
		success = false;

		if (list_empty(&deferred))
			return 0;

		list_for_each_entry_safe(dev, tmp, &deferred, active) {
			list_del(&dev->active);
			INIT_LIST_HEAD(&dev->active);

			dev_dbg(dev, "re-probe device\n");
			bus_for_each_driver(dev->bus, drv) {
				if (match(drv, dev))
					continue;
				success = true;
				break;
			}
		}
	} while (success);

	list_for_each_entry(dev, &deferred, active)
		dev_report_permanent_probe_deferral(dev);

	return 0;
}
late_initcall(device_probe_deferred);

struct driver *get_driver_by_name(const char *name)
{
	struct driver *drv;

	for_each_driver(drv) {
		if(!strcmp(name, drv->name))
			return drv;
	}

	return NULL;
}

int register_driver(struct driver *drv)
{
	struct device *dev = NULL;

	if (!drv->name)
		return -EINVAL;

	debug("register_driver: %s\n", drv->name);

	BUG_ON(!drv->bus);

	list_add_tail(&drv->list, &driver_list);
	list_add_tail(&drv->bus_list, &drv->bus->driver_list);

	bus_for_each_device(drv->bus, dev)
		match(drv, dev);

	return 0;
}
EXPORT_SYMBOL(register_driver);

void unregister_driver(struct driver *drv)
{
	struct device *dev;

	list_del(&drv->list);
	list_del(&drv->bus_list);

	bus_for_each_device(drv->bus, dev) {
		if (dev->driver == drv) {
			device_remove(dev);
			dev->driver = NULL;
			list_del(&dev->active);
			INIT_LIST_HEAD(&dev->active);
		}
	}
}

struct resource *dev_get_resource(struct device *dev, unsigned long type,
				  int num)
{
	int i, n = 0;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) == type) {
			if (n == num)
				return res;
			n++;
		}
	}

	return ERR_PTR(-ENOENT);
}

void *dev_get_mem_region(struct device *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, IORESOURCE_MEM, num);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return (void __force *)res->start;
}
EXPORT_SYMBOL(dev_get_mem_region);

struct resource *dev_get_resource_by_name(struct device *dev,
					  unsigned long type,
					  const char *name)
{
	int i;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *res = &dev->resource[i];
		if (resource_type(res) != type)
			continue;
		if (!res->name)
			continue;
		if (!strcmp(name, res->name))
			return res;
	}

	return ERR_PTR(-ENOENT);
}

static struct resource *dev_request_iomem_resource(struct device *dev,
						   const struct resource *res)
{
	return request_iomem_region(dev_name(dev), res->start, res->end);
}

int dev_request_resource(struct device *dev, const struct resource *res)
{
	return PTR_ERR_OR_ZERO(dev_request_iomem_resource(dev, res));
}
EXPORT_SYMBOL(dev_request_resource);

struct resource *dev_request_mem_resource_by_name(struct device *dev, const char *name)
{
	struct resource *res;

	res = dev_get_resource_by_name(dev, IORESOURCE_MEM, name);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return dev_request_iomem_resource(dev, res);
}
EXPORT_SYMBOL(dev_request_mem_resource_by_name);

void __iomem *dev_request_mem_region_by_name(struct device *dev,
					     const char *name)
{
	struct resource *res;

	res = dev_request_mem_resource_by_name(dev, name);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region_by_name);

void __iomem *dev_platform_get_and_ioremap_resource(struct device *dev,
						    int num,
						    struct resource **out_res)
{
	struct resource *res;

	res = dev_request_mem_resource(dev, num);
	if (IS_ERR(res))
		return IOMEM_ERR_PTR(PTR_ERR(res));

	/* As everything is mapped 1:1 by default, drivers on unlucky
	 * platforms can end up successfully requesting memory, but
	 * getting a base address that looks like an error pointer.
	 * Let's warn loudly about this case, as these drivers
	 * should be using dev_request_mem_resource instead.
	 */
	if (WARN_ON(IS_ERR_VALUE(res->start)))
		return IOMEM_ERR_PTR(-EINVAL);

	if (out_res)
		*out_res = res;

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_platform_get_and_ioremap_resource);

struct resource *dev_request_mem_resource(struct device *dev, int num)
{
	struct resource *res;

	res = dev_get_resource(dev, IORESOURCE_MEM, num);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return dev_request_iomem_resource(dev, res);
}

void __iomem *dev_request_mem_region_err_null(struct device *dev, int num)
{
	struct resource *res;

	res = dev_request_mem_resource(dev, num);
	if (IS_ERR(res) || WARN_ON(!res->start))
		return NULL;

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region_err_null);

void __iomem *dev_request_mem_region(struct device *dev, int num)
{
	struct resource *res;

	res = dev_request_mem_resource(dev, num);
	if (!IS_ERR(res) && WARN_ON(IS_ERR_VALUE(res->start)))
		return IOMEM_ERR_PTR(res->start);
	if (IS_ERR(res))
		return ERR_CAST(res);

	return IOMEM(res->start);
}
EXPORT_SYMBOL(dev_request_mem_region);

int generic_memmap_rw(struct cdev *cdev, void **map, int flags)
{
	if (!cdev->dev)
		return -EINVAL;

	*map = dev_get_mem_region(cdev->dev, 0);

	return PTR_ERR_OR_ZERO(*map);
}

int generic_memmap_ro(struct cdev *cdev, void **map, int flags)
{
	if (flags & PROT_WRITE)
		return -EACCES;

	return generic_memmap_rw(cdev, map, flags);
}

/**
 * dev_add_alias - add alias for device
 * @dev: device
 * @fmt: format string for the device's alias
 */
int dev_add_alias(struct device *dev, const char *fmt, ...)
{
	va_list va, va_copy;
	unsigned int len;
	struct device_alias *alias;

	va_start(va, fmt);
	va_copy(va_copy, va);
	len = vsnprintf(NULL, 0, fmt, va_copy);
	va_end(va_copy);

	alias = malloc(struct_size(alias, name, len + 1));
	if (!alias)
		return -ENOMEM;

	vsnprintf(alias->name, len + 1, fmt, va);

	va_end(va);

	alias->dev = dev;
	list_add_tail(&alias->list, &device_alias_list);

	return 0;
}
EXPORT_SYMBOL_GPL(dev_add_alias);

bool device_remove(struct device *dev)
{
	if (dev->bus && dev->bus->remove)
		dev->bus->remove(dev);
	else if (dev->driver->remove)
		dev->driver->remove(dev);
	else
		return false; /* nothing to do */

	return true;
}
EXPORT_SYMBOL_GPL(device_remove);

static void devices_shutdown(void)
{
	struct device *dev;

	list_for_each_entry(dev, &active_device_list, active) {
		if (device_remove(dev))
			pr_report_probe("%*sremove-> %s\n", 1 * 4, "", dev_name(dev));
		dev->driver = NULL;
	}
}
devshutdown_exitcall(devices_shutdown);

const void *device_get_match_data(struct device *dev)
{
	if (dev->of_id_entry)
		return dev->of_id_entry->data;

	if (dev->id_entry)
		return (void *)dev->id_entry->driver_data;

	return NULL;
}
EXPORT_SYMBOL(device_get_match_data);

static void device_set_deferred_probe_reason(struct device *dev,
					     const struct va_format *vaf)
{
	char *reason;
	char *last_char;

	free(dev->deferred_probe_reason);

	reason = xasprintf("%pV", vaf);

	/* drop newline char at end of reason string */
	last_char = reason + strlen(reason) - 1;

	if (*last_char == '\n')
		*last_char = '\0';

	dev->deferred_probe_reason = reason;
}

/**
 * dev_err_probe - probe error check and log helper
 * @loglevel: log level configured in source file
 * @dev: the pointer to the struct device
 * @err: error value to test
 * @fmt: printf-style format string
 * @...: arguments as specified in the format string
 *
 * This helper implements common pattern present in probe functions for error
 * checking: print debug or error message depending if the error value is
 * -EPROBE_DEFER and propagate error upwards.
 *
 * In case of -EPROBE_DEFER it sets the device's deferred_probe_reason attribute,
 * but does not report an error. The error is recorded and displayed later, if
 * (and only if) the probe is permanently deferred. For all other error codes,
 * it just outputs the error along with the formatted message.
 *
 * It replaces code sequence::
 *
 * 	if (err != -EPROBE_DEFER)
 * 		dev_err(dev, ...);
 * 	else
 * 		dev_dbg(dev, ...);
 * 	return err;
 *
 * with::
 *
 * 	return dev_err_probe(dev, err, ...);
 *
 * Returns @err.
 *
 */
int dev_err_probe(struct device *dev, int err, const char *fmt, ...);
int dev_err_probe(struct device *dev, int err, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;

	if (err == -EPROBE_DEFER)
		device_set_deferred_probe_reason(dev, &vaf);

	dev_printf(err == -EPROBE_DEFER ? MSG_DEBUG : MSG_ERR,
		   dev, "error %pe: %pV", ERR_PTR(err), &vaf);

	va_end(args);

	return err;
}
EXPORT_SYMBOL_GPL(dev_err_probe);

/*
 * device_find_child - device iterator for locating a particular device.
 * @parent: parent struct device
 * @match: Callback function to check device
 * @data: Data to pass to match function
 *
 * The callback should return 0 if the device doesn't match and non-zero
 * if it does.  If the callback returns non-zero and a reference to the
 * current device can be obtained, this function will return to the caller
 * and not iterate over any more devices.
 */
struct device *device_find_child(struct device *parent, void *data,
				 int (*match)(struct device *dev, void *data))
{
	struct device *child;

	if (!parent)
		return NULL;

	list_for_each_entry(child, &parent->children, sibling) {
		if (match(child, data))
			return child;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(device_find_child);

#ifdef CONFIG_CMD_DEVINFO
struct device_info_cb {
	void  (*info) (struct device *);
	struct list_head list;
};

static void devinfo_init(struct device *dev)
{
	/* We initialize on demand, because devinfo_add should be
	 * callable even before device registration
	 */
	if (!dev->info_list.prev && !dev->info_list.next)
		INIT_LIST_HEAD(&dev->info_list);
}

void devinfo_add(struct device *dev, void (*info)(struct device *))
{
	struct device_info_cb *cb = xmalloc(sizeof(*cb));

	devinfo_init(dev);

	cb->info = info;
	list_add_tail(&cb->list, &dev->info_list);
}

void devinfo_del(struct device *dev, void (*info)(struct device *))
{
	struct device_info_cb *cb, *tmp;

	devinfo_init(dev);

	list_for_each_entry_safe(cb, tmp, &dev->info_list, list) {
		if (cb->info == info) {
			list_del(&cb->list);
			free(cb);
		}
	}
}

void devinfo(struct device *dev)
{
	struct device_info_cb *cb;

	devinfo_init(dev);

	list_for_each_entry(cb, &dev->info_list, list)
		cb->info(dev);
}
#endif
