// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Message Protocol bus layer
 *
 * Copyright (C) 2018-2021 ARM Ltd.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <driver.h>

#include "common.h"

static DEFINE_IDR(scmi_protocols);

static const struct scmi_device_id *
scmi_dev_match_id(struct scmi_device *scmi_dev, struct scmi_driver *scmi_drv)
{
	const struct scmi_device_id *id = scmi_drv->id_table;

	if (!id)
		return NULL;

	for (; id->protocol_id; id++)
		if (id->protocol_id == scmi_dev->protocol_id) {
			if (!id->name)
				return id;
			else if (!strcmp(id->name, scmi_dev->name))
				return id;
		}

	return NULL;
}

static int scmi_dev_match(struct device_d *dev, struct driver_d *drv)
{
	struct scmi_driver *scmi_drv = to_scmi_driver(drv);
	struct scmi_device *scmi_dev = to_scmi_dev(dev);
	const struct scmi_device_id *id;

	id = scmi_dev_match_id(scmi_dev, scmi_drv);
	if (id)
		return 0;

	return -1;
}

static int scmi_match_by_id_table(struct device_d *dev, void *data)
{
	struct scmi_device *sdev = to_scmi_dev(dev);
	struct scmi_device_id *id_table = data;

	return sdev->protocol_id == id_table->protocol_id &&
		!strcmp(sdev->name, id_table->name);
}

struct scmi_device *scmi_child_dev_find(struct device_d *parent,
					int prot_id, const char *name)
{
	struct scmi_device_id id_table;
	struct device_d *dev;

	id_table.protocol_id = prot_id;
	id_table.name = name;

	dev = device_find_child(parent, &id_table, scmi_match_by_id_table);
	if (!dev)
		return NULL;

	return to_scmi_dev(dev);
}

const struct scmi_protocol *scmi_protocol_get(int protocol_id)
{
	const struct scmi_protocol *proto;

	proto = idr_find(&scmi_protocols, protocol_id);
	if (!proto) {
		pr_warn("SCMI Protocol 0x%x not found!\n", protocol_id);
		return NULL;
	}

	pr_debug("Found SCMI Protocol 0x%x\n", protocol_id);

	return proto;
}

static int scmi_dev_probe(struct device_d *dev)
{
	struct scmi_driver *scmi_drv = to_scmi_driver(dev->driver);
	struct scmi_device *scmi_dev = to_scmi_dev(dev);
	const struct scmi_device_id *id;

	id = scmi_dev_match_id(scmi_dev, scmi_drv);
	if (!id)
		return -ENODEV;

	if (!scmi_dev->handle)
		return -EPROBE_DEFER;

	return scmi_drv->probe(scmi_dev);
}

static void scmi_dev_remove(struct device_d *dev)
{
	struct scmi_driver *scmi_drv = to_scmi_driver(dev->driver);
	struct scmi_device *scmi_dev = to_scmi_dev(dev);

	if (scmi_drv->remove)
		scmi_drv->remove(scmi_dev);
}

static struct bus_type scmi_bus_type = {
	.name =	"scmi_protocol",
	.match = scmi_dev_match,
	.probe = scmi_dev_probe,
	.remove = scmi_dev_remove,
};

int scmi_driver_register(struct scmi_driver *driver)
{
	int retval;

	retval = scmi_protocol_device_request(driver->id_table);
	if (retval)
		return retval;

	driver->driver.bus = &scmi_bus_type;
	driver->driver.name = driver->name;

	retval = register_driver(&driver->driver);
	if (!retval)
		pr_debug("registered new scmi driver %s\n", driver->name);

	return retval;
}
EXPORT_SYMBOL_GPL(scmi_driver_register);

struct scmi_device *
scmi_device_alloc(struct device_node *np, struct device_d *parent, int protocol,
		   const char *name)
{
	struct scmi_device *scmi_dev;

	scmi_dev = kzalloc(sizeof(*scmi_dev), GFP_KERNEL);
	if (!scmi_dev)
		return NULL;

	scmi_dev->name = kstrdup_const(name ?: "unknown", GFP_KERNEL);
	if (!scmi_dev->name) {
		kfree(scmi_dev);
		return NULL;
	}

	scmi_dev->dev.id = DEVICE_ID_DYNAMIC;
	scmi_dev->protocol_id = protocol;
	scmi_dev->dev.parent = parent;
	scmi_dev->dev.device_node = np;
	scmi_dev->dev.bus = &scmi_bus_type;
	dev_set_name(&scmi_dev->dev, "scmi_dev");

	return scmi_dev;
}

void scmi_device_destroy(struct scmi_device *scmi_dev)
{
	kfree_const(scmi_dev->name);
	unregister_device(&scmi_dev->dev);
}

void scmi_set_handle(struct scmi_device *scmi_dev)
{
	scmi_dev->handle = scmi_handle_get(&scmi_dev->dev);
}

int scmi_protocol_register(const struct scmi_protocol *proto)
{
	int ret;

	if (!proto) {
		pr_err("invalid protocol\n");
		return -EINVAL;
	}

	if (!proto->instance_init) {
		pr_err("missing init for protocol 0x%x\n", proto->id);
		return -EINVAL;
	}

	ret = idr_alloc_one(&scmi_protocols, (void *)proto, proto->id);
	if (ret != proto->id) {
		pr_err("unable to allocate SCMI idr slot for 0x%x - err %d\n",
		       proto->id, ret);
		return ret;
	}

	pr_debug("Registered SCMI Protocol 0x%x\n", proto->id);

	return 0;
}
EXPORT_SYMBOL_GPL(scmi_protocol_register);

void scmi_protocol_unregister(const struct scmi_protocol *proto)
{
	idr_remove(&scmi_protocols, proto->id);

	pr_debug("Unregistered SCMI Protocol 0x%x\n", proto->id);

	return;
}
EXPORT_SYMBOL_GPL(scmi_protocol_unregister);

int __init scmi_bus_init(void)
{
	int retval;

	retval = bus_register(&scmi_bus_type);
	if (retval)
		pr_err("scmi protocol bus register failed (%d)\n", retval);

	return retval;
}
