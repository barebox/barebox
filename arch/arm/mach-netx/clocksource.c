/*
 *
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix 
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
#include <init.h>
#include <clock.h>
#include <mach/netx-regs.h>

uint64_t netx_clocksource_read(void)
{
	return GPIO_REG(GPIO_COUNTER_CURRENT(0));
}

static struct clocksource cs = {
	.read	= netx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int clocksource_init (void)
{
	/* disable timer initially */
	GPIO_REG(GPIO_COUNTER_CTRL(0)) = 0;
	/* Reset the timer value to zero */
	GPIO_REG(GPIO_COUNTER_CURRENT(0)) = 0;
	GPIO_REG(GPIO_COUNTER_MAX(0)) = 0xffffffff;
	GPIO_REG(GPIO_COUNTER_CTRL(0)) = COUNTER_CTRL_RUN;

	cs.mult = clocksource_hz2mult(100 * 1000 * 1000, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
