// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Ahmad Fatoum
 */
#define pr_fmt(fmt)	"bootdef: " fmt

#include <boot.h>
#include <xfuncs.h>
#include <string.h>
#include <bootsource.h>
#include <driver.h>
#include <init.h>

static int bootdev_process(struct cdev *cdev, void *entries)
{
	return bootentry_create_from_name(entries, cdev->name);
}

static int bootdef_add_entry(struct bootentries *entries, const char *name)
{
	int ret;

	ret = cdev_alias_resolve_for_each(name, bootdev_process, entries);
	if (ret == -ENODEV) {
		pr_info("Could not autodetect bootsource device\n");
		return 0;
	}

	return ret;
}

static struct bootentry_provider bootdef_entry_provider = {
	.generate = bootdef_add_entry,
};

static int bootdef_entry_init(void)
{
	return bootentry_register_provider(&bootdef_entry_provider);
}
device_initcall(bootdef_entry_init);
