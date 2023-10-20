// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2022 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <fcntl.h>
#include <linux/regmap.h>
#include <linux/bitfield.h>
#include <linux/export.h>

#include "internal.h"

enum { MULTI_MAP_8, MULTI_MAP_16, MULTI_MAP_32, MULTI_MAP_64, MULTI_MAP_COUNT };
struct regmap_multi {
	struct cdev cdev;
	struct regmap *map[MULTI_MAP_COUNT];
};

static struct regmap *regmap_multi_cdev_get_map(struct cdev *cdev, unsigned rwsize)
{
	struct regmap_multi *multi = container_of(cdev, struct regmap_multi, cdev);

	switch (rwsize) {
		case 8:
			return multi->map[MULTI_MAP_64];
		case 4:
			return multi->map[MULTI_MAP_32];
		case 2:
			return multi->map[MULTI_MAP_16];
		case 1:
			return multi->map[MULTI_MAP_8];
	}

	return NULL;
}

static ssize_t regmap_multi_cdev_read(struct cdev *cdev, void *buf, size_t count,
				      loff_t offset, unsigned long flags)
{
	unsigned rwsize = FIELD_GET(O_RWSIZE_MASK, flags);
	struct regmap *map;

	map = regmap_multi_cdev_get_map(cdev, rwsize);
	if (!map)
		return -EINVAL;

	count = ALIGN_DOWN(count, rwsize);
	return regmap_bulk_read(map, offset, buf, count / rwsize) ?: count;
}

static ssize_t regmap_multi_cdev_write(struct cdev *cdev, const void *buf, size_t count,
				       loff_t offset, unsigned long flags)
{
	unsigned rwsize = FIELD_GET(O_RWSIZE_MASK, flags);
	struct regmap *map;

	map = regmap_multi_cdev_get_map(cdev, rwsize);
	if (!map)
		return -EINVAL;

	count = ALIGN_DOWN(count, rwsize);
	return regmap_bulk_write(map, offset, buf, count / rwsize) ?: count;
}

static struct cdev_operations regmap_multi_fops = {
	.read	= regmap_multi_cdev_read,
	.write	= regmap_multi_cdev_write,
};

int regmap_multi_register_cdev(struct regmap *map8,
			       struct regmap *map16,
			       struct regmap *map32,
			       struct regmap *map64)
{
	struct regmap *maps[MULTI_MAP_COUNT] = { map8, map16, map32, map64 };
	struct regmap_multi *multi;
	struct cdev *cdev;
	int i;

	multi = xzalloc(sizeof(*multi));
	cdev = &multi->cdev;

	cdev->ops = &regmap_multi_fops;
	cdev->size = LLONG_MAX;

	for (i = 0; i < MULTI_MAP_COUNT; i++) {
		if (!maps[i])
			continue;

		multi->map[i] = maps[i];
		cdev->size = min_t(loff_t, regmap_size_bytes(maps[i]), cdev->size);
		cdev->dev = cdev->dev ?: maps[i]->dev;
	}

	if (!cdev->dev) {
		free(multi);
		return -EINVAL;
	}

	cdev->name = xstrdup(dev_name(cdev->dev));

	return devfs_create(cdev);
}
