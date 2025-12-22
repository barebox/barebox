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
	struct list_head list;
	bool populated;
};

static int sba_add_partitions(struct cdev *rcdev, void *data)
{
	struct sba *sba = data;
	int ret;

	if (sba->populated)
		return 0;

	dev_info(sba->dev, "Adding %s -> %s\n", sba->alias, rcdev->name);

	ret = devfs_add_alias_node(rcdev, sba->alias, sba->dev->device_node);
	if (ret)
		return ret;

	sba->populated = true;

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
