// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <block.h>
#include <disks.h>
#include <fcntl.h>
#include <xfuncs.h>

#include <linux/kstrtox.h>

#include "dm-target.h"

struct dm_linear {
	struct cdev *cdev;
	loff_t offset;
};

static int dm_linear_read(struct dm_target *ti, void *buf,
			  sector_t block, blkcnt_t num_blocks)
{
	struct dm_linear *l = ti->private;
	ssize_t ret;

	block <<= SECTOR_SHIFT;
	num_blocks <<= SECTOR_SHIFT;

	ret = cdev_read(l->cdev, buf, num_blocks, l->offset + block, 0);
	if (ret < num_blocks)
		return (ret < 0) ? ret : -EIO;

	return 0;
}

static int dm_linear_write(struct dm_target *ti, const void *buf,
			   sector_t block, blkcnt_t num_blocks)
{
	struct dm_linear *l = ti->private;
	ssize_t ret;

	block <<= SECTOR_SHIFT;
	num_blocks <<= SECTOR_SHIFT;

	ret = cdev_write(l->cdev, buf, num_blocks, l->offset + block, 0);
	if (ret < num_blocks)
		return (ret < 0) ? ret : -EIO;

	return 0;
}

static int dm_linear_create(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_linear *l;
	int err;

	if (argc != 2) {
		dm_target_err(ti, "Expected 2 arguments (\"<dev> <offset>\"), got %u\n",
			      argc);
		err = -EINVAL;
		goto err;
	}

	l = xzalloc(sizeof(*l));
	ti->private = l;

	if (kstrtoull(argv[1], 0, &l->offset)) {
		dm_target_err(ti, "Invalid offset: \"%s\"\n", argv[1]);
		err = -EINVAL;
		goto err_free;
	}
	l->offset <<= SECTOR_SHIFT;

	l->cdev = cdev_open_by_path_name(argv[0], O_RDWR);
	if (!l->cdev) {
		dm_target_err(ti, "Cannot open device %s: %m\n", argv[0]);
		err = -ENODEV;
		goto err_free;
	}

	l->cdev = cdev_readlink(l->cdev);

	if ((ti->size << SECTOR_SHIFT) > (l->cdev->size - l->offset)) {
		dm_target_err(ti, "%s is too small to map %llu blocks at %llu, %llu available\n",
			      argv[0], ti->size, l->offset >> SECTOR_SHIFT,
			      (l->cdev->size - l->offset) >> SECTOR_SHIFT);
		err = -ENOSPC;
		goto err_close;
	}

	return 0;

err_close:
	cdev_close(l->cdev);
err_free:
	free(l);
err:
	return err;
}

static int dm_linear_destroy(struct dm_target *ti)
{
	struct dm_linear *l = ti->private;

	cdev_close(l->cdev);
	free(l);
	return 0;
}

static char *dm_linear_asprint(struct dm_target *ti)
{
	struct dm_linear *l = ti->private;

	return xasprintf("dev:%s offset:%llu",
			 cdev_name(l->cdev), l->offset >> SECTOR_SHIFT);
}

static struct dm_target_ops dm_linear_ops = {
	.name = "linear",
	.asprint = dm_linear_asprint,
	.create = dm_linear_create,
	.destroy = dm_linear_destroy,
	.read = dm_linear_read,
	.write = dm_linear_write,
};

static int __init dm_linear_init(void)
{
	return dm_target_register(&dm_linear_ops);
}
device_initcall(dm_linear_init);
