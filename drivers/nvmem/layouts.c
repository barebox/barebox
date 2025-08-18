// SPDX-License-Identifier: GPL-2.0
/*
 * NVMEM layout bus handling
 *
 * Copyright (C) 2023 Bootlin
 * Author: Miquel Raynal <miquel.raynal@bootlin.com
 */

#include <device.h>
#include <of_device.h>
#include <module.h>
#include <linux/nvmem-consumer.h>
#include <linux/nvmem-provider.h>

#include "internals.h"

#define to_nvmem_layout_driver(drv) \
	(container_of_const((drv), struct nvmem_layout_driver, driver))
#define to_nvmem_layout_device(_dev) \
	container_of((_dev), struct nvmem_layout, dev)

static int nvmem_layout_bus_match(struct device *dev, const struct device_driver *drv)
{
	return of_driver_match_device(dev, drv);
}

static int nvmem_layout_bus_probe(struct device *dev)
{
	struct nvmem_layout_driver *drv = to_nvmem_layout_driver(dev->driver);
	struct nvmem_layout *layout = to_nvmem_layout_device(dev);

	if (!drv->probe || !drv->remove)
		return -EINVAL;

	return drv->probe(layout);
}

static void nvmem_layout_bus_remove(struct device *dev)
{
	struct nvmem_layout_driver *drv = to_nvmem_layout_driver(dev->driver);
	struct nvmem_layout *layout = to_nvmem_layout_device(dev);

	return drv->remove(layout);
}

static struct bus_type nvmem_layout_bus_type = {
	.name		= "nvmem-layout",
	.match		= nvmem_layout_bus_match,
	.probe		= nvmem_layout_bus_probe,
	.remove		= nvmem_layout_bus_remove,
};

int nvmem_layout_driver_register(struct nvmem_layout_driver *drv)
{
	drv->driver.bus = &nvmem_layout_bus_type;

	return register_driver(&drv->driver);
}
EXPORT_SYMBOL_GPL(nvmem_layout_driver_register);

static int nvmem_layout_create_device(struct nvmem_device *nvmem,
				      struct device_node *np)
{
	struct nvmem_layout *layout;
	struct device *dev;
	int ret;

	layout = kzalloc(sizeof(*layout), GFP_KERNEL);
	if (!layout)
		return -ENOMEM;

	/* Create a bidirectional link */
	layout->nvmem = nvmem;
	nvmem->layout = layout;

	/* Device model registration */
	dev = &layout->dev;
	dev->parent = &nvmem->dev;
	dev->bus = &nvmem_layout_bus_type;
	dev->dma_mask = DMA_BIT_MASK(32);
	dev->of_node = of_node_get(np);
	of_device_make_bus_id(dev);

	ret = register_device(dev);
	if (ret) {
		put_device(dev);
		return ret;
	}

	np->dev = dev;

	return 0;
}

static const struct of_device_id of_nvmem_layout_skip_table[] = {
	{ .compatible = "fixed-layout", },
	{}
};

static int nvmem_layout_bus_populate(struct nvmem_device *nvmem,
				     struct device_node *layout_dn)
{
	int ret;

	/* Make sure it has a compatible property */
	if (!of_property_present(layout_dn, "compatible")) {
		pr_debug("%s() - skipping %pOF, no compatible prop\n",
			 __func__, layout_dn);
		return 0;
	}

	/* Fixed layouts are parsed manually somewhere else for now */
	if (of_match_node(of_nvmem_layout_skip_table, layout_dn)) {
		pr_debug("%s() - skipping %pOF node\n", __func__, layout_dn);
		return 0;
	}

	if (layout_dn->dev) {
		pr_debug("%s() - skipping %pOF, already populated\n",
			 __func__, layout_dn);

		return 0;
	}

	/* NVMEM layout buses expect only a single device representing the layout */
	ret = nvmem_layout_create_device(nvmem, layout_dn);
	if (ret)
		return ret;

	return 0;
}

struct device_node *of_nvmem_layout_get_container(struct nvmem_device *nvmem)
{
	struct device_node *np = nvmem->dev.of_node;

	if (!np)
		return NULL;

	return of_get_child_by_name(np, "nvmem-layout");
}
EXPORT_SYMBOL_GPL(of_nvmem_layout_get_container);

/*
 * Returns the number of devices populated, 0 if the operation was not relevant
 * for this nvmem device, an error code otherwise.
 */
int nvmem_populate_layout(struct nvmem_device *nvmem)
{
	struct device_node *layout_dn;
	int ret;

	layout_dn = of_nvmem_layout_get_container(nvmem);
	if (!layout_dn)
		return 0;

	/* Populate the layout device */
	ret = nvmem_layout_bus_populate(nvmem, layout_dn);

	of_node_put(layout_dn);
	return ret;
}

void nvmem_destroy_layout(struct nvmem_device *nvmem)
{
	struct device *dev;

	if (!nvmem->layout)
		return;

	dev = &nvmem->layout->dev;
	unregister_device(dev);
}

int nvmem_layout_bus_register(void)
{
	return bus_register(&nvmem_layout_bus_type);
}
