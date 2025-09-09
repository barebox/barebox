// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <block.h>
#include <disks.h>
#include <device-mapper.h>
#include <stdio.h>
#include <string.h>
#include <xfuncs.h>

#include <linux/kstrtox.h>

#include "dm-target.h"

static LIST_HEAD(dm_target_ops_list);

static struct dm_target_ops *dm_target_ops_find(const char *name)
{
	struct dm_target_ops *ops;

	list_for_each_entry(ops, &dm_target_ops_list, list) {
		if (!strcmp(ops->name, name))
			return ops;
	}
	return NULL;
}

int dm_target_register(struct dm_target_ops *ops)
{
	list_add(&ops->list, &dm_target_ops_list);
	return 0;
}

void dm_target_unregister(struct dm_target_ops *ops)
{
	list_del(&ops->list);
}

struct dm_device {
	struct device dev;
	struct block_device blk;
	struct list_head targets;
};

DEFINE_DEV_CLASS(dm_class, "dm");

struct dm_device *dm_find_by_name(const char *name)
{
	struct device *dev;

	class_for_each_device(&dm_class, dev) {
		if (!strcmp(dev_name(dev), name))
			return container_of(dev, struct dm_device, dev);
	}

	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(dm_find_by_name);

int dm_foreach(int (*cb)(struct dm_device *dm, void *ctx), void *ctx)
{
	struct dm_device *dm;
	int err;

	class_for_each_container_of_device(&dm_class, dm, dev) {
		err = cb(dm, ctx);
		if (err)
			return err;
	}

	return 0;
}
EXPORT_SYMBOL(dm_foreach);

void dm_target_err(struct dm_target *ti, const char *fmt, ...)
{
	struct dm_target *iter;
	va_list ap;
	char *msg;
	int i = 0;

	/* Figure out the index in the target list, which corresponds
	 * to the first column in the table generated in
	 * dm_asprint().
	 */
	list_for_each_entry(iter, &ti->dm->targets, list) {
		if (iter == ti)
			break;
		i++;
	}

	va_start(ap, fmt);
	msg = xvasprintf(fmt, ap);
	va_end(ap);

	dev_err(&ti->dm->dev, "%d(%s): %s", i, ti->ops->name, msg);
	free(msg);
}
EXPORT_SYMBOL(dm_target_err);

static int dm_blk_read(struct block_device *blk, void *buf,
		       sector_t block, blkcnt_t num_blocks)
{
	struct dm_device *dm = container_of(blk, struct dm_device, blk);
	struct dm_target *ti;
	blkcnt_t tnblks, todo;
	sector_t tblk;
	int err;

	todo = num_blocks;

	/* We can have multiple non-overlapping targets and a read may
	 * span multiple targets. Since targets are ordered by base
	 * address, we iterate until we find the first applicable
	 * target and then read as much as we can from each until we
	 * have completed the full request.
	 */
	list_for_each_entry(ti, &dm->targets, list) {
		if (block < ti->base || block >= ti->base + ti->size)
			continue;

		if (!ti->ops->read)
			return -EIO;

		tblk = block - ti->base;
		tnblks = min(todo, ti->size - tblk);
		err = ti->ops->read(ti, buf, tblk, tnblks);
		if (err)
			return err;

		block += tnblks;
		todo -= tnblks;
		buf += tnblks << SECTOR_SHIFT;
		if (!todo)
			return 0;
	}

	return -EIO;
}

static int dm_blk_write(struct block_device *blk, const void *buf,
			sector_t block, blkcnt_t num_blocks)
{
	struct dm_device *dm = container_of(blk, struct dm_device, blk);
	struct dm_target *ti;
	blkcnt_t tnblks, todo;
	sector_t tblk;
	int err;

	todo = num_blocks;

	list_for_each_entry(ti, &dm->targets, list) {
		if (block < ti->base || block >= ti->base + ti->size)
			continue;

		if (!ti->ops->write)
			return -EIO;

		tblk = block - ti->base;
		tnblks = min(todo, ti->size - tblk);
		err = ti->ops->write(ti, buf, tblk, tnblks);
		if (err)
			return err;

		block += tnblks;
		todo -= tnblks;
		buf += tnblks << SECTOR_SHIFT;
		if (!todo)
			return 0;
	}

	return -EIO;
}

static struct block_device_ops dm_blk_ops = {
	.read = dm_blk_read,
	.write = dm_blk_write,
};

static blkcnt_t dm_size(struct dm_device *dm)
{
	struct dm_target *last;

	if (list_empty(&dm->targets))
		return 0;

	last = list_last_entry(&dm->targets, struct dm_target, list);
	return last->base + last->size;
}

char *dm_asprint(struct dm_device *dm)
{
	struct dm_target *ti;
	char *str, *tistr;
	int n = 0;

	str = xasprintf(
		"Device: %s\n"
		"Size: %llu\n"
		"Table:\n"
		" #       Start         End        Size  Target\n",
		dev_name(&dm->dev), dm_size(dm));

	list_for_each_entry(ti, &dm->targets, list) {
		tistr = ti->ops->asprint ? ti->ops->asprint(ti) : NULL;

		str = xrasprintf(str, "%2d  %10llu  %10llu  %10llu  %s %s\n",
				 n++, ti->base, ti->base + ti->size - 1,
				 ti->size, ti->ops->name, tistr ? : "");

		if (tistr)
			free(tistr);
	}

	return str;
}
EXPORT_SYMBOL(dm_asprint);

static void dm_devinfo(struct device *dev)
{
	struct dm_device *dm = dev->priv;
	char *info = dm_asprint(dm);

	puts(info);
	free(info);
}

static struct dm_target *dm_parse_row(struct dm_device *dm, const char *crow)
{
	struct dm_target *ti = NULL;
	char *row, **argv;
	int argc;

	row = xstrdup(crow);
	argv = strtokv(row, " \t", &argc);

	if (argc < 3) {
		dev_err(&dm->dev, "Invalid row: \"%s\"\n", crow);
		goto err;
	}

	ti = xzalloc(sizeof(*ti));
	ti->dm = dm;

	ti->ops = dm_target_ops_find(argv[2]);
	if (!ti->ops) {
		dev_err(&dm->dev, "Unknown target: \"%s\"\n", argv[2]);
		goto err;
	}

	if (kstrtoull(argv[0], 0, &ti->base)) {
		dm_target_err(ti, "Invalid start: \"%s\"\n", argv[0]);
		goto err;
	}

	if (ti->base != dm_size(dm)) {
		/* Could we just skip the start argument, then? Seems
		 * like it, but let's keep things compatible with the
		 * table format in Linux.
		 */
		dm_target_err(ti, "Non-contiguous start: %llu, expected %llu\n",
			      ti->base, dm_size(dm));
		goto err;
	}

	if (kstrtoull(argv[1], 0, &ti->size) || !ti->size) {
		dm_target_err(ti, "Invalid length: \"%s\"\n", argv[1]);
		goto err;
	}

	argc -= 3;

	if (ti->ops->create(ti, argc, argc ? &argv[3] : NULL))
		goto err;

	free(argv);
	free(row);
	return ti;

err:
	free(ti);

	free(argv);
	free(row);
	return NULL;
}

static int dm_parse_table(struct dm_device *dm, const char *ctable)
{
	struct dm_target *ti, *tmp;
	char *table, **rowv;
	int i, rowc;

	table = xstrdup(ctable);
	rowv = strtokv(table, "\n", &rowc);

	for (i = 0; i < rowc; i++) {
		ti = dm_parse_row(dm, rowv[i]);
		if (!ti)
			goto err_destroy;

		list_add_tail(&ti->list, &dm->targets);
	}

	free(rowv);
	free(table);
	return 0;

err_destroy:
	list_for_each_entry_safe_reverse(ti, tmp, &dm->targets, list) {
		ti->ops->destroy(ti);
		list_del(&ti->list);
	}

	free(rowv);
	free(table);

	dev_err(&dm->dev, "Failed to parse table\n");
	return -EINVAL;
}

void dm_destroy(struct dm_device *dm)
{
	struct dm_target *ti;

	blockdevice_unregister(&dm->blk);

	list_for_each_entry_reverse(ti, &dm->targets, list) {
		ti->ops->destroy(ti);
	}

	unregister_device(&dm->dev);

	free(dm);
}
EXPORT_SYMBOL(dm_destroy);

struct dm_device *dm_create(const char *name, const char *table)
{
	struct dm_target *ti;
	struct dm_device *dm;
	int err;

	dm = xzalloc(sizeof(*dm));
	dm->dev.priv = dm;

	dev_set_name(&dm->dev, "%s", name);
	dm->dev.id = DEVICE_ID_SINGLE;
	err = register_device(&dm->dev);
	if (err)
		goto err_free;

	class_add_device(&dm_class, &dm->dev);
	devinfo_add(&dm->dev, dm_devinfo);

	INIT_LIST_HEAD(&dm->targets);
	err = dm_parse_table(dm, table);
	if (err)
		goto err_unregister;

	dm->blk = (struct block_device) {
		.dev = &dm->dev,
		.cdev.name = xstrdup(name),

		.type = BLK_TYPE_VIRTUAL,
		.ops = &dm_blk_ops,

		.num_blocks = dm_size(dm),

		/* Linux defines a fixed sector size of 512B for DM
		 * devices.
		 */
		.blockbits = SECTOR_SHIFT,
	};

	err = blockdevice_register(&dm->blk);
	if (err)
		goto err_destroy;

	return dm;

err_destroy:
	list_for_each_entry_reverse(ti, &dm->targets, list) {
		ti->ops->destroy(ti);
	}

err_unregister:
	unregister_device(&dm->dev);

err_free:
	free(dm);
	return ERR_PTR(err);
}
EXPORT_SYMBOL(dm_create);
