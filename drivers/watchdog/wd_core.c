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

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <watchdog.h>

/*
 * Note: this simple framework supports one watchdog only.
 */
static struct watchdog *watchdog;

int watchdog_register(struct watchdog *wd)
{
	if (watchdog != NULL)
		return -EBUSY;

	watchdog = wd;
	return 0;
}
EXPORT_SYMBOL(watchdog_register);

int watchdog_deregister(struct watchdog *wd)
{
	if (watchdog == NULL || wd != watchdog)
		return -ENODEV;

	watchdog = NULL;
	return 0;
}
EXPORT_SYMBOL(watchdog_deregister);

/*
 * start, stop or retrigger the watchdog
 * timeout in [seconds]. timeout of '0' will disable the watchdog (if possible)
 */
int watchdog_set_timeout(unsigned timeout)
{
	if (watchdog == NULL)
		return -ENODEV;

	return watchdog->set_timeout(watchdog, timeout);
}
EXPORT_SYMBOL(watchdog_set_timeout);
