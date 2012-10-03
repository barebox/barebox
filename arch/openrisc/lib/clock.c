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
#include <asm/system.h>
#include <asm/openrisc_exc.h>

static uint64_t openrisc_clocksource_read(void)
{
	return (uint64_t)(mfspr(SPR_TTCR));
}

static struct clocksource cs = {
	.read	= openrisc_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 12,
};

static int clocksource_init(void)
{
	mtspr(SPR_TTMR, SPR_TTMR_CR | 0xFFFFFF);
	cs.mult = clocksource_hz2mult(OPENRISC_TIMER_FREQ, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
