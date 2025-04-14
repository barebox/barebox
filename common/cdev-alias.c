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
#include <init.h>

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

static struct cdev_alias cdev_alias_aliases[] = {
	{ "bootsource", cdev_alias_resolve_bootsource },
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
