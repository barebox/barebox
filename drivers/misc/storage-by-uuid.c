// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <envfs.h>
#include <fs.h>

static LIST_HEAD(sbu_list);

struct sbu {
	char *uuid;
	struct device *dev;
	struct cdev *rcdev;
	struct cdev cdev;
	struct list_head list;
};

void storage_by_uuid_check_exist(struct cdev *cdev);

static ssize_t sbu_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	struct sbu *sbu = cdev->priv;

	return cdev_read(sbu->rcdev, buf, count, offset, flags);
}

static ssize_t sbu_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	struct sbu *sbu = cdev->priv;

	return cdev_write(sbu->rcdev, buf, count, offset, flags);
}

static int sbu_ioctl(struct cdev *cdev, unsigned int request, void *buf)
{
	struct sbu *sbu = cdev->priv;

	return cdev_ioctl(sbu->rcdev, request, buf);
}

static int sbu_open(struct cdev *cdev, unsigned long flags)
{
	struct sbu *sbu = cdev->priv;

	return cdev_open(sbu->rcdev, flags);
}

static int sbu_close(struct cdev *cdev)
{
	struct sbu *sbu = cdev->priv;

	cdev_close(sbu->rcdev);

	return 0;
}

static int sbu_flush(struct cdev *cdev)
{
	struct sbu *sbu = cdev->priv;

	return cdev_flush(sbu->rcdev);
}

static int sbu_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct sbu *sbu = cdev->priv;

	return cdev_erase(sbu->rcdev, count, offset);
}

static int sbu_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	struct sbu *sbu = cdev->priv;

	return cdev_protect(sbu->rcdev, count, offset, prot);
}

static int sbu_discard_range(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct sbu *sbu = cdev->priv;

	return cdev_discard_range(sbu->rcdev, count, offset);
}

static int sbu_memmap(struct cdev *cdev, void **map, int flags)
{
	struct sbu *sbu = cdev->priv;

	return cdev_memmap(sbu->rcdev, map, flags);
}

static int sbu_truncate(struct cdev *cdev, size_t size)
{
	struct sbu *sbu = cdev->priv;

	return cdev_truncate(sbu->rcdev, size);
}

static struct cdev_operations sbu_ops = {
	.read = sbu_read,
	.write = sbu_write,
	.ioctl = sbu_ioctl,
	.open = sbu_open,
	.close = sbu_close,
	.flush = sbu_flush,
	.erase = sbu_erase,
	.protect = sbu_protect,
	.discard_range = sbu_discard_range,
	.memmap = sbu_memmap,
	.truncate = sbu_truncate,
};

static void storage_by_uuid_add_partitions(struct sbu *sbu, struct cdev *rcdev)
{
	int ret;

	if (sbu->rcdev)
		return;

	sbu->rcdev = rcdev;
	sbu->cdev.name = sbu->uuid;
	sbu->cdev.size = rcdev->size;
	sbu->cdev.ops = &sbu_ops;
	sbu->cdev.dev = sbu->dev;
	sbu->cdev.priv = sbu;

	ret = devfs_create(&sbu->cdev);
	if (ret) {
		dev_err(sbu->dev, "Failed to create cdev: %s\n", strerror(-ret));
		return;
	}

	of_parse_partitions(&sbu->cdev, sbu->dev->of_node);
}

static void check_exist(struct sbu *sbu)
{
	struct cdev *cdev;

	for_each_cdev(cdev) {
		if (!strcmp(cdev->diskuuid, sbu->uuid)) {
			dev_dbg(sbu->dev, "Found %s %s\n", cdev->name, cdev->diskuuid);
			storage_by_uuid_add_partitions(sbu, cdev);
		}
	}
}

static int sbu_detect(struct device *dev)
{
	struct sbu *sbu = dev->priv;

	dev_dbg(dev, "%s\n", __func__);

	check_exist(sbu);

	return 0;
}

static int storage_by_uuid_probe(struct device *dev)
{
	struct sbu *sbu;
	int ret;
	const char *uuid;

	sbu = xzalloc(sizeof(*sbu));

	ret = of_property_read_string(dev->of_node, "uuid", &uuid);
	if (ret)
		return ret;

	sbu->dev = dev;
	sbu->uuid = xstrdup(uuid);

	list_add_tail(&sbu->list, &sbu_list);

	check_exist(sbu);
	dev->priv = sbu;
	dev->detect = sbu_detect;

	return 0;
}

static struct of_device_id storage_by_uuid_dt_ids[] = {
	{
		.compatible = "barebox,storage-by-uuid",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, storage_by_uuid_dt_ids);

static struct driver storage_by_uuid_driver = {
	.name		= "storage-by-uuid",
	.probe		= storage_by_uuid_probe,
	.of_compatible	= storage_by_uuid_dt_ids,
};
device_platform_driver(storage_by_uuid_driver);
