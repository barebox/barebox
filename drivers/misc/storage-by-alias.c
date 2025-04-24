// SPDX-License-Identifier: GPL-2.0-only
/*
 * cdev aliases for existing block devices. In future, this should
 * likely be replaced by a more generic device mapper support.
 *
 * Currently supported targets:
 * - barebox,storage-by-uuid
 *   Useful for referencing existing EFI disks and their partition
 *   from device tree by offset
 * - barebox,bootsource
 *   Reference the boot medium indicated by barebox $bootsource
 *   and $bootsource_instance variable
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <envfs.h>
#include <fs.h>

static LIST_HEAD(sba_list);

struct sba {
	char *alias;
	struct device *dev;
	struct cdev *rcdev;
	struct cdev cdev;
	struct list_head list;
};

static ssize_t sba_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	struct sba *sba = cdev->priv;

	return cdev_read(sba->rcdev, buf, count, offset, flags);
}

static ssize_t sba_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	struct sba *sba = cdev->priv;

	return cdev_write(sba->rcdev, buf, count, offset, flags);
}

static int sba_ioctl(struct cdev *cdev, unsigned int request, void *buf)
{
	struct sba *sba = cdev->priv;

	return cdev_ioctl(sba->rcdev, request, buf);
}

static int sba_open(struct cdev *cdev, unsigned long flags)
{
	struct sba *sba = cdev->priv;

	return cdev_open(sba->rcdev, flags);
}

static int sba_close(struct cdev *cdev)
{
	struct sba *sba = cdev->priv;

	cdev_close(sba->rcdev);

	return 0;
}

static int sba_flush(struct cdev *cdev)
{
	struct sba *sba = cdev->priv;

	return cdev_flush(sba->rcdev);
}

static int sba_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct sba *sba = cdev->priv;

	return cdev_erase(sba->rcdev, count, offset);
}

static int sba_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	struct sba *sba = cdev->priv;

	return cdev_protect(sba->rcdev, count, offset, prot);
}

static int sba_discard_range(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct sba *sba = cdev->priv;

	return cdev_discard_range(sba->rcdev, count, offset);
}

static int sba_memmap(struct cdev *cdev, void **map, int flags)
{
	struct sba *sba = cdev->priv;

	return cdev_memmap(sba->rcdev, map, flags);
}

static int sba_truncate(struct cdev *cdev, size_t size)
{
	struct sba *sba = cdev->priv;

	return cdev_truncate(sba->rcdev, size);
}

static struct cdev_operations sba_ops = {
	.read = sba_read,
	.write = sba_write,
	.ioctl = sba_ioctl,
	.open = sba_open,
	.close = sba_close,
	.flush = sba_flush,
	.erase = sba_erase,
	.protect = sba_protect,
	.discard_range = sba_discard_range,
	.memmap = sba_memmap,
	.truncate = sba_truncate,
};

static int sba_add_partitions(struct cdev *rcdev, void *data)
{
	struct sba *sba = data;
	int ret;

	dev_dbg(sba->dev, "Adding %s -> %s\n", sba->alias, rcdev->name);

	if (sba->rcdev)
		return 0;

	sba->rcdev = rcdev;
	sba->cdev.link = rcdev;
	sba->cdev.name = sba->alias;
	sba->cdev.size = rcdev->size;
	sba->cdev.ops = &sba_ops;
	sba->cdev.dev = sba->dev;
	sba->cdev.priv = sba;

	if (rcdev->flags & DEVFS_WRITE_AUTOERASE)
		sba->cdev.flags |= DEVFS_WRITE_AUTOERASE;

	ret = devfs_create(&sba->cdev);
	if (ret) {
		dev_err(sba->dev, "Failed to create cdev: %pe\n", ERR_PTR(ret));
		return 0;
	}

	of_parse_partitions(&sba->cdev, sba->dev->of_node);
	return 0;
}

static void check_exist(struct sba *sba)
{
	cdev_alias_resolve_for_each(sba->alias, sba_add_partitions, sba);
}

static int sba_detect(struct device *dev)
{
	struct sba *sba = dev->priv;

	dev_dbg(dev, "%s\n", __func__);

	check_exist(sba);

	return 0;
}

static int storage_by_uuid_init(struct sba *sba)
{
	const char *uuid;
	int ret;

	ret = of_property_read_string(sba->dev->of_node, "uuid", &uuid);
	if (ret)
		return ret;

	sba->alias = xasprintf("diskuuid.%s", uuid);

	return 0;
}

static int storage_by_bootsource_init(struct sba *sba)
{
	sba->alias = xasprintf("bootsource");
	return 0;
}

static int sba_probe(struct device *dev)
{
	int (*init)(struct sba *);
	struct sba *sba;
	int ret;

	sba = xzalloc(sizeof(*sba));
	sba->dev = dev;

	init = device_get_match_data(dev);
	if (!init)
		return -ENODEV;

	ret = init(sba);
	if (ret)
		return ret;

	list_add_tail(&sba->list, &sba_list);

	check_exist(sba);
	dev->priv = sba;
	dev->detect = sba_detect;

	return 0;
}

static struct of_device_id sba_dt_ids[] = {
	{
		.compatible = "barebox,storage-by-uuid",
		.data = storage_by_uuid_init
	}, {
		.compatible = "barebox,bootsource",
		.data = storage_by_bootsource_init
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, sba_dt_ids);

static struct driver sba_driver = {
	.name		= "storage-by-alias",
	.probe		= sba_probe,
	.of_compatible	= sba_dt_ids,
};
device_platform_driver(sba_driver);
