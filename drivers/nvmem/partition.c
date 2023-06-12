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

struct nvmem_device *nvmem_partition_register(struct cdev *cdev)
{
	struct nvmem_config config = {};

	config.name = cdev->name;
	config.dev = cdev->dev;
	config.cdev = cdev;
	config.priv = cdev;
	config.stride = 1;
	config.word_size = 1;
	config.size = cdev->size;
	config.reg_read = nvmem_cdev_read;
	config.reg_write = nvmem_cdev_write;

	return nvmem_register(&config);
}
