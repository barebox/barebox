/*
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

#ifndef INCLUDE_WATCHDOG_H
# define INCLUDE_WATCHDOG_H

#include <poller.h>

struct watchdog {
	int (*set_timeout)(struct watchdog *, unsigned);
	const char *name;
	struct device_d *hwdev;
	struct device_d dev;
	unsigned int priority;
	unsigned int timeout_max;
	unsigned int timeout_cur;
	unsigned int poller_enable;
	struct poller_async poller;
	struct list_head list;
};

#ifdef CONFIG_WATCHDOG
int watchdog_register(struct watchdog *);
int watchdog_deregister(struct watchdog *);
int watchdog_set_timeout(unsigned);
unsigned int of_get_watchdog_priority(struct device_node *node);
#else
static inline int watchdog_register(struct watchdog *w)
{
	return 0;
}

static inline int watchdog_deregister(struct watchdog *w)
{
	return 0;
}

static inline int watchdog_set_timeout(unsigned t)
{
	return 0;
}

static inline unsigned int of_get_watchdog_priority(struct device_node *node)
{
	return 0;
}
#endif

#define WATCHDOG_DEFAULT_PRIORITY 100

#endif /* INCLUDE_WATCHDOG_H */
