/*
 * clock.c - wrapper between a barebox clocksource and linux
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <mach/linux.h>

static uint64_t linux_clocksource_read(void)
{
	return linux_get_time();
}

static struct clocksource cs = {
	.read	= linux_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int clocksource_init (void)
{
	cs.mult = clocksource_hz2mult(1000 * 1000 * 1000, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
