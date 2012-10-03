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
#include <asm/hardware.h>
#include <mach/at91_pit.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/io.h>
#include <io.h>

uint64_t at91sam9_clocksource_read(void)
{
	return at91_sys_read(AT91_PIT_PIIR);
}

static struct clocksource cs = {
	.read	= at91sam9_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int clocksource_init (void)
{
	/*
	 * Enable PITC Clock
	 * The clock is already enabled for system controller in boot
	 */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91_ID_SYS);

	/* Enable PITC */
	at91_sys_write(AT91_PIT_MR, 0xfffff | AT91_PIT_PITEN);

	cs.mult = clocksource_hz2mult(1000000 * 6, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
