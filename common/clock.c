/*
 * clock.c - generic clocksource implementation
 *
 * This file contains the clocksource implementation from the Linux
 * kernel originally by John Stultz
 *
 * Copyright (C) 2004, 2005 IBM, John Stultz (johnstul@us.ibm.com)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <asm-generic/div64.h>
#include <clock.h>

static struct clocksource *current_clock;
static uint64_t time_ns;

/**
 * get_time_ns - get current timestamp in nanoseconds
 */
uint64_t get_time_ns(void)
{
	struct clocksource *cs = current_clock;
        uint64_t cycle_now, cycle_delta;
        uint64_t ns_offset;

        /* read clocksource: */
	cycle_now = cs->read() & cs->mask;

        /* calculate the delta since the last call: */
        cycle_delta = (cycle_now - cs->cycle_last) & cs->mask;

        /* convert to nanoseconds: */
        ns_offset = cyc2ns(cs, cycle_delta);

	cs->cycle_last = cycle_now;

	time_ns += ns_offset;
        return time_ns;
}
EXPORT_SYMBOL(get_time_ns);

/**
 * clocksource_hz2mult - calculates mult from hz and shift
 * @hz:                 Clocksource frequency in Hz
 * @shift_constant:     Clocksource shift factor
 *
 * Helper functions that converts a hz counter
 * frequency to a timsource multiplier, given the
 * clocksource shift value
 */
uint32_t clocksource_hz2mult(uint32_t hz, uint32_t shift_constant)
{
        /*  hz = cyc/(Billion ns)
         *  mult/2^shift  = ns/cyc
         *  mult = ns/cyc * 2^shift
         *  mult = 1Billion/hz * 2^shift
         *  mult = 1000000000 * 2^shift / hz
         *  mult = (1000000000<<shift) / hz
         */
        uint64_t tmp = ((uint64_t)1000000000) << shift_constant;

        tmp += hz/2; /* round for do_div */
        do_div(tmp, hz);

        return (uint32_t)tmp;
}

int is_timeout(uint64_t start_ns, uint64_t time_offset_ns)
{
	if ((int64_t)(start_ns + time_offset_ns - get_time_ns()) < 0)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_timeout);

void ndelay(unsigned long nsecs)
{
	uint64_t start = get_time_ns();

	while(!is_timeout(start, nsecs));
}
EXPORT_SYMBOL(ndelay);

void udelay(unsigned long usecs)
{
	uint64_t start = get_time_ns();

	while(!is_timeout(start, usecs * USECOND));
}
EXPORT_SYMBOL(udelay);

void mdelay(unsigned long msecs)
{
	uint64_t start = get_time_ns();

	while(!is_timeout(start, msecs * MSECOND));
}
EXPORT_SYMBOL(mdelay);

int init_clock(struct clocksource *cs)
{
	current_clock = cs;
	return 0;
}
