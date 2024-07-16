// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <fcntl.h>
#include <init.h>

static ssize_t port_read(struct cdev *cdev, void *buf, size_t count, loff_t offset,
		 unsigned long flags)
{
	struct device *dev = cdev->dev;
	ssize_t size;
	int rwsize = (flags & (O_RWSIZE_1 | O_RWSIZE_2 | O_RWSIZE_4));

	if (!dev)
		return -1;


	if (!dev || dev->num_resources < 1)
		return -1;

	if (resource_size(&dev->resource[0]) > 0 || offset != 0)
		count = min_t(size_t, count,
			      resource_size(&dev->resource[0]) - offset);

	size = count;

	/* no rwsize specification given. Use maximum */
	if (!rwsize)
		rwsize = 4;

	rwsize = rwsize >> O_RWSIZE_SHIFT;

	count = ALIGN_DOWN(count, rwsize);

	while (count) {
		switch (rwsize) {
		case 1:
			*((u8 *)buf) = inb(offset);
			break;
		case 2:
			*((u16 *)buf) = inw(offset);
			break;
		case 4:
			*((u32  *)buf) = inl(offset);
			break;
		}
		buf += rwsize;
		offset += rwsize;
		count -= rwsize;
	}

	return size;
}

static ssize_t port_write(struct cdev *cdev, const void *buf, size_t count,
			  loff_t offset, unsigned long flags)
{
	struct device *dev = cdev->dev;
	ssize_t size;
	int rwsize = (flags & (O_RWSIZE_1 | O_RWSIZE_2 | O_RWSIZE_4));

	if (!dev)
		return -1;

	if (!dev || dev->num_resources < 1)
		return -1;

	if (resource_size(&dev->resource[0]) > 0 || offset != 0)
		count = min_t(size_t, count, resource_size(&dev->resource[0]) - offset);
	size = count;

	/* no rwsize specification given. Use maximum */
	if (!rwsize)
		rwsize = 4;

	rwsize = rwsize >> O_RWSIZE_SHIFT;

	count = ALIGN_DOWN(count, rwsize);

	while (count) {
		switch (rwsize) {
		case 1:
			outb(*((u8 *)buf), offset);
			break;
		case 2:
			outw(*((u16 *)buf), offset);
			break;
		case 4:
			outl(*((u32 *)buf), offset);
			break;
		}
		buf += rwsize;
		offset += rwsize;
		count -= rwsize;
	}

	return size;
}

static struct cdev_operations portops = {
	.read  = port_read,
	.write = port_write,
};

static int port_probe(struct device *dev)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));
	dev->priv = cdev;

	cdev->name = (char*)dev->resource[0].name;
	cdev->size = resource_size(&dev->resource[0]);

	cdev->ops = &portops;
	cdev->dev = dev;

	devfs_create(cdev);

	return 0;
}

static struct driver port_drv = {
	.name  = "port",
	.probe = port_probe,
};

static int port_init(void)
{
	struct device *dev;
	struct resource res = {
		.start = 0,
		.end = IO_SPACE_LIMIT,
		.flags = IORESOURCE_IO,
		.name = "port",
	};
	int ret;

	dev = device_alloc("port", DEVICE_ID_DYNAMIC);
	if (!dev)
		return -ENOMEM;

	dev->resource = xmemdup(&res, sizeof(res));
	dev->num_resources = 1;

	ret = platform_device_register(dev);
	if (ret)
		return ret;

	pr_debug("I/O port base %p\n", PCI_IOBASE);

	return platform_driver_register(&port_drv);
}
device_initcall(port_init);
