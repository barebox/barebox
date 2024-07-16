/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <mach/linux.h>
#include <asm/io.h>

static LIST_HEAD(sandbox_device_list);

int sandbox_add_device(struct device *dev)
{
	list_add(&dev->list, &sandbox_device_list);

	return 0;
}

static int sandbox_device_init(void)
{
	struct device *dev, *tmp;

	list_for_each_entry_safe(dev, tmp, &sandbox_device_list, list) {
		/* reset the list_head before registering for real */
		dev->list.prev = NULL;
		dev->list.next = NULL;
		platform_device_register(dev);
	}

	return 0;
}
postcore_initcall(sandbox_device_init);
