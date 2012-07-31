/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
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
#include <clock.h>
#include <init.h>
#include <asm/nios2-io.h>
#include <io.h>

static struct nios_timer *timer = (struct nios_timer *)NIOS_SOPC_TIMER_BASE;

static uint64_t nios_clocksource_read(void)
{
	uint64_t value;

	writew(0x5555, &timer->snapl);       /* Dummy value*/
	value = (uint64_t)((readw(&timer->snaph) << 16) + readw(&timer->snapl));

	return ~value;
}

static struct clocksource cs = {
	.read	= nios_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 12,
};

static int clocksource_init(void)
{
	writew(0, &timer->control);
	writew(0xFFFF, &timer->periodl);
	writew(0xFFFF, &timer->periodh);
	writew(NIOS_TIMER_CONT | NIOS_TIMER_START, &timer->control);

	cs.mult = clocksource_hz2mult(NIOS_SOPC_TIMER_FREQ, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);

