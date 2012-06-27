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

struct watchdog {
	int (*set_timeout)(struct watchdog *, unsigned);
};

int watchdog_register(struct watchdog *);
int watchdog_deregister(struct watchdog *);
int watchdog_set_timeout(unsigned);

#endif /* INCLUDE_WATCHDOG_H */
