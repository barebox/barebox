// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Ahmad Fatoum
 */
#define pr_fmt(fmt)	"cdev-alias: " fmt

#include <xfuncs.h>
#include <string.h>
#include <stringlist.h>
#include <bootsource.h>
#include <driver.h>
#include <block.h>
#include <init.h>
#include <linux/bits.h>

struct cdev_alias {
	const char *name;
	int (*resolve)(struct cdev_alias *, const char *arg,
		       cdev_alias_processor_t fn, void *data);
};

static int cdev_alias_resolve_bootsource(struct cdev_alias *cdev_alias,
					 const char *partname,
					 cdev_alias_processor_t fn,
					 void *data)
{
	struct cdev *cdev;

	cdev = bootsource_of_cdev_find();
	if (!cdev)
		return -ENODEV;

	if (partname) {
		cdev = cdev_find_partition(cdev, partname);
		if (!cdev)
			return -ENODEV;
	}

	return fn(cdev, data);
}

static int cdev_alias_resolve_diskuuid(struct cdev_alias *cdev_alias,
				       const char *uuid,
				       cdev_alias_processor_t fn,
				       void *data)
{
	struct cdev *cdev;
	char *str, *arg;
	int ret = 0;

	arg = str = xstrdup(uuid);
	uuid = strsep(&arg, ".");
	if (!uuid || !*uuid) {
		ret = -EINVAL;
		goto out;
	}

	for_each_cdev(cdev) {
		if (cdev_is_partition(cdev))
			continue;

		if (strcasecmp(cdev->diskuuid, uuid))
			continue;

		if (arg) {
			cdev = cdev_find_partition(cdev, arg);
			if (!cdev) {
				ret = -ENODEV;
				break;
			}
		}

		ret = fn(cdev, data);
		break;
	}

out:
	free(str);
	return ret;
}

#define STORAGE_REMOVABLE	BIT(0)
#define STORAGE_BUILTIN		BIT(1)

/**
 * call_for_each_storage() - invoke callback for each storage medium
 *
 * @fn:			callback to invoke
 * @data:		callback-specific data
 * @filter:		OR-ed types of STORAGE_* to filter for
 * @only_bootsource:	If true, include only bootsource if available,
 *			otherwise omit always
 * Return:		number of successful callback invocations or a negative error
 */
static int call_for_each_storage(cdev_alias_processor_t fn,
				 void *data,
				 unsigned filter,
				 bool only_bootsource)
{
	struct cdev *cdev, *bootcdev;
	int ret, nmatches = 0;

	bootcdev = bootsource_of_cdev_find();

	for_each_cdev(cdev) {
		struct block_device *bdev;

		if (!cdev_is_storage(cdev) || cdev_is_partition(cdev))
			continue;

		bdev = cdev_get_block_device(cdev);

		if (((filter & STORAGE_REMOVABLE) && bdev && bdev->removable) ||
		    ((filter & STORAGE_BUILTIN) && (!bdev || !bdev->removable))) {
			if (only_bootsource && cdev != bootcdev)
				continue;
			if (!only_bootsource && cdev == bootcdev)
				continue;

			ret = fn(cdev, data);
			if (ret < 0)
				return ret;
			nmatches++;

			/* Got our bootsource, no need to continue iteration */
			if (only_bootsource)
				break;
		}
	}

	return nmatches;
}

static int cdev_alias_resolve_storage(struct cdev_alias_res *cdev_alias_res,
				      const char *class,
				      cdev_alias_processor_t fn,
				      void *data)
{
	struct cdev *bootcdev;
	unsigned filter = 0;
	int bootsource, nmatches;

	if (!class)
		filter = ~0;
	else if (streq_ptr(class, "removable"))
		filter |= STORAGE_REMOVABLE;
	else if (streq_ptr(class, "builtin"))
		filter |= STORAGE_BUILTIN;
	else
		return -EINVAL;

	bootcdev = bootsource_of_cdev_find();

	bootsource = call_for_each_storage(fn, data, filter, true);
	if (bootsource < 0)
		return bootsource;

	nmatches = call_for_each_storage(fn, data, filter, false);
	if (nmatches < 0)
		return nmatches;

	return bootsource + nmatches;
}

static struct cdev_alias cdev_alias_aliases[] = {
	{ "bootsource", cdev_alias_resolve_bootsource },
	{ "diskuuid", cdev_alias_resolve_diskuuid },
	{ "storage", cdev_alias_resolve_storage },
	{ /* sentinel */}
};

int cdev_alias_resolve_for_each(const char *name,
				cdev_alias_processor_t fn, void *data)
{
	struct cdev_alias *alias;
	int ret = 0;
	char *buf, *arg;

	arg = buf = xstrdup(name);
	name = strsep(&arg, ".");

	for (alias = cdev_alias_aliases; alias->name; alias++) {
		if (!streq_ptr(name, alias->name))
			continue;

		ret = alias->resolve(alias, arg, fn, data);
		break;
	}

	free(buf);
	return ret;
}
