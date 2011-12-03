/*
 * (C) Copyright 2009 Sascha Hauer <s.hauer@pengutronix.de>
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
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/io.h>

#define OSCR	0x40A00010

uint64_t pxa_clocksource_read(void)
{
	return readl(OSCR);
}

static struct clocksource cs = {
	.read	= pxa_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 20,
};

static int clocksource_init(void)
{
	cs.mult = clocksource_hz2mult(3250000, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
