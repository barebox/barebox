// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/nvmem-provider.h>

static int nvmem_cdev_write(void *ctx, unsigned offset, const void *val, size_t bytes)
{
	return cdev_write(ctx, val, bytes, offset, 0);
}

static int nvmem_cdev_read(void *ctx, unsigned offset, void *buf, size_t bytes)
{
	return cdev_read(ctx, buf, bytes, offset, 0);
}

static int nvmem_cells_probe(struct device *dev)
{
	struct nvmem_config config = {};
	struct device_node *node = dev->of_node;
	struct cdev *cdev;

	cdev = cdev_by_device_node(node);
	if (!cdev)
		return -EINVAL;

	config.name = cdev->name;
	config.dev = cdev->dev;
	config.cdev = cdev;
	config.priv = cdev;
	config.stride = 1;
	config.word_size = 1;
	config.size = cdev->size;
	config.reg_read = nvmem_cdev_read;
	config.reg_write = nvmem_cdev_write;

	return PTR_ERR_OR_ZERO(nvmem_register(&config));
}

static __maybe_unused struct of_device_id nvmem_cells_dt_ids[] = {
	{ .compatible = "nvmem-cells", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nvmem_cells_dt_ids);

static struct driver nvmem_cells_driver = {
	.name	= "nvmem_cells",
	.probe	= nvmem_cells_probe,
	.of_compatible = nvmem_cells_dt_ids,
};
device_platform_driver(nvmem_cells_driver);
