/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "watchdog: " fmt

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <watchdog.h>

static LIST_HEAD(watchdog_list);

static const char *watchdog_name(struct watchdog *wd)
{
	if (wd->dev)
		return dev_name(wd->dev);
	if (wd->name)
		return wd->name;

	return "unknown";
}

int watchdog_register(struct watchdog *wd)
{
	if (!wd->priority)
		wd->priority = WATCHDOG_DEFAULT_PRIORITY;

	list_add_tail(&wd->list, &watchdog_list);

	pr_debug("registering watchdog %s with priority %d\n", watchdog_name(wd),
			wd->priority);

	return 0;
}
EXPORT_SYMBOL(watchdog_register);

int watchdog_deregister(struct watchdog *wd)
{
	list_del(&wd->list);

	return 0;
}
EXPORT_SYMBOL(watchdog_deregister);

static struct watchdog *watchdog_get_default(void)
{
	struct watchdog *tmp, *wd = NULL;
	int priority = 0;

	list_for_each_entry(tmp, &watchdog_list, list) {
		if (tmp->priority > priority) {
			priority = tmp->priority;
			wd = tmp;
		}
	}

	return wd;
}

/*
 * start, stop or retrigger the watchdog
 * timeout in [seconds]. timeout of '0' will disable the watchdog (if possible)
 */
int watchdog_set_timeout(unsigned timeout)
{
	struct watchdog *wd;

	wd = watchdog_get_default();

	if (!wd)
		return -ENODEV;

	pr_debug("setting timeout on %s to %ds\n", watchdog_name(wd), timeout);

	return wd->set_timeout(wd, timeout);
}
EXPORT_SYMBOL(watchdog_set_timeout);

/**
 * of_get_watchdog_priority() - get the desired watchdog priority from device tree
 * @node:	The device_node to read the property from
 *
 * return: The priority
 */
unsigned int of_get_watchdog_priority(struct device_node *node)
{
	unsigned int priority = WATCHDOG_DEFAULT_PRIORITY;

	of_property_read_u32(node, "watchdog-priority", &priority);

	return priority;
}
