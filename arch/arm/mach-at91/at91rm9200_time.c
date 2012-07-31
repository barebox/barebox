/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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
#include <mach/hardware.h>
#include <mach/at91_tc.h>
#include <mach/at91_st.h>
#include <mach/at91_pmc.h>
#include <mach/io.h>
#include <io.h>

/*
 * The ST_CRTR is updated asynchronously to the master clock ... but
 * the updates as seen by the CPU don't seem to be strictly monotonic.
 * Waiting until we read the same value twice avoids glitching.
 */
uint64_t at91rm9200_clocksource_read(void)
{
	unsigned long x1, x2;

	x1 = at91_sys_read(AT91_ST_CRTR);
	do {
		x2 = at91_sys_read(AT91_ST_CRTR);
		if (x1 == x2)
			break;
		x1 = x2;
	} while (1);
	return x1;
}

static struct clocksource cs = {
	.mask	= CLOCKSOURCE_MASK(20),
	.read	= at91rm9200_clocksource_read,
	.shift	= 10,
};

static int clocksource_init (void)
{
	/* The 32KiHz "Slow Clock" (tick every 30517.58 nanoseconds) is used
	 * directly for the clocksource and all clockevents, after adjusting
	 * its prescaler from the 1 Hz default.
	 */
	at91_sys_write(AT91_ST_RTMR, 1);

	cs.mult = clocksource_hz2mult(AT91_SLOW_CLOCK, cs.shift);

	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);

/*
 * Reset the cpu through the reset controller
 */
void __noreturn reset_cpu (unsigned long ignored)
{
	/*
	 * Perform a hardware reset with the use of the Watchdog timer.
	 */
	at91_sys_write(AT91_ST_WDMR, AT91_ST_RSTEN | AT91_ST_EXTEN | 1);
	at91_sys_write(AT91_ST_CR, AT91_ST_WDRST);

	/* Not reached */
	while (1);
}
EXPORT_SYMBOL(reset_cpu);
