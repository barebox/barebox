// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <block.h>
#include <disks.h>
#include <fcntl.h>
#include <xfuncs.h>

#include <linux/kstrtox.h>

#include "dm-target.h"

struct dm_linear {
	struct dm_cdev dmcdev;
};

static int dm_linear_read(struct dm_target *ti, void *buf,
			  sector_t block, blkcnt_t num_blocks)
{
	struct dm_linear *l = ti->private;

	return dm_cdev_read(&l->dmcdev, buf, block, num_blocks);
}

static int dm_linear_write(struct dm_target *ti, const void *buf,
			   sector_t block, blkcnt_t num_blocks)
{
	struct dm_linear *l = ti->private;

	return dm_cdev_write(&l->dmcdev, buf, block, num_blocks);
}

static int dm_linear_create(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_linear *l = NULL;
	loff_t offset;
	char *errmsg;
	int err;

	if (argc != 2) {
		dm_target_err(ti, "Expected 2 arguments (\"<dev> <offset>\"), got %u\n",
			      argc);
		err = -EINVAL;
		goto err;
	}

	l = xzalloc(sizeof(*l));
	ti->private = l;

	if (kstrtoull(argv[1], 0, &offset)) {
		dm_target_err(ti, "Invalid offset: \"%s\"\n", argv[1]);
		err = -EINVAL;
		goto err;
	}

	err = dm_cdev_open(&l->dmcdev, argv[0], O_RDWR, offset,
			   ti->size, SECTOR_SIZE, &errmsg);
	if (err) {
		dm_target_err(ti, "Cannot open device %s: %s\n", argv[0], errmsg);
		free(errmsg);
		goto err;
	}

	return 0;

err:
	if (l)
		free(l);

	return err;
}

static int dm_linear_destroy(struct dm_target *ti)
{
	struct dm_linear *l = ti->private;

	dm_cdev_close(&l->dmcdev);
	free(l);
	return 0;
}

static char *dm_linear_asprint(struct dm_target *ti)
{
	struct dm_linear *l = ti->private;

	return xasprintf("dev:%s offset:%llu",
			 cdev_name(l->dmcdev.cdev), l->dmcdev.blk.start);
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
