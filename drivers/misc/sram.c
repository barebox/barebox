// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/misc/sram.c - generic memory mapped SRAM driver
 */

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/err.h>

struct sram {
	struct resource *res;
	char *name;
	struct cdev cdev;
};

static struct cdev_operations memops = {
	.read  = mem_read,
	.write = mem_write,
	.memmap = generic_memmap_rw,
};

static int sram_probe(struct device_d *dev)
{
	struct resource *iores;
	struct sram *sram;
	struct resource *res;
	void __iomem *base;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	sram = xzalloc(sizeof(*sram));

	sram->cdev.name = basprintf("sram%d", cdev_find_free_index("sram"));

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	sram->cdev.size = (unsigned long)resource_size(res);
	sram->cdev.ops = &memops;
	sram->cdev.dev = dev;

	ret = devfs_create(&sram->cdev);
	if (ret)
		return ret;

	return 0;
}

static __maybe_unused struct of_device_id sram_dt_ids[] = {
	{
		.compatible = "mmio-sram",
	}, {
	},
};

static struct driver_d sram_driver = {
	.name = "mmio-sram",
	.probe = sram_probe,
	.of_compatible = sram_dt_ids,
};
device_platform_driver(sram_driver);
