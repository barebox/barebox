// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Nicolas Saenz Julienne <nsaenzjulienne@suse.de>
 */

#include <io.h>
#include <driver.h>
#include <linux/nvmem-provider.h>
#include <init.h>

struct rmem {
	struct device_d *dev;
	const struct resource *mem;
};

static int rmem_read(void *context, unsigned int offset,
		     void *val, size_t bytes)
{
	struct rmem *rmem = context;
	return mem_copy(rmem->dev, val, (void *)rmem->mem->start + offset,
			bytes, offset, 0);
}

static struct nvmem_bus rmem_nvmem_bus = {
	.read = rmem_read,
};

static int rmem_probe(struct device_d *dev)
{
	struct nvmem_config config = { };
	struct resource *mem;
	struct rmem *priv;

	mem = dev_request_mem_resource(dev, 0);
	if (IS_ERR(mem))
		return PTR_ERR(mem);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->mem = mem;

	config.dev = priv->dev = dev;
	config.priv = priv;
	config.name = "rmem";
	config.size = resource_size(mem);
	config.bus = &rmem_nvmem_bus;

	return PTR_ERR_OR_ZERO(nvmem_register(&config));
}

static const struct of_device_id rmem_match[] = {
	{ .compatible = "nvmem-rmem", },
	{ /* sentinel */ },
};

static struct driver_d rmem_driver = {
	.name = "rmem",
	.of_compatible = rmem_match,
	.probe = rmem_probe,
};
device_platform_driver(rmem_driver);

MODULE_AUTHOR("Nicolas Saenz Julienne <nsaenzjulienne@suse.de>");
MODULE_DESCRIPTION("Reserved Memory Based nvmem Driver");
MODULE_LICENSE("GPL");
