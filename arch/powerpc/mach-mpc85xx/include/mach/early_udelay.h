/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>

/* early_udelay: delay execution before timers are initialized
 *
 * "usecs * 100" gives a time of around 1 second on a 1Ghz CPU.
 */
static inline  void early_udelay(unsigned long usecs)
{
	uint64_t start;
	uint32_t loops = usecs * 100;

	start = get_ticks();

	while ((get_ticks() - start) < loops)
		;
}
