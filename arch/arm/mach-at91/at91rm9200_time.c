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
#include <restart.h>
#include <mach/hardware.h>
#include <mach/at91rm9200_st.h>
#include <io.h>

static void __iomem *st = IOMEM(AT91RM9200_BASE_ST);

/*
 * The ST_CRTR is updated asynchronously to the master clock ... but
 * the updates as seen by the CPU don't seem to be strictly monotonic.
 * Waiting until we read the same value twice avoids glitching.
 */
uint64_t at91rm9200_clocksource_read(void)
{
	unsigned long x1, x2;

	x1 = readl(st + AT91RM9200_ST_CRTR);
	do {
		x2 = readl(st + AT91RM9200_ST_CRTR);
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
	writel(1, st + AT91RM9200_ST_RTMR);

	cs.mult = clocksource_hz2mult(AT91_SLOW_CLOCK, cs.shift);

	return init_clock(&cs);
}
core_initcall(clocksource_init);

/*
 * Reset the cpu through the reset controller
 */
static void __noreturn at91rm9200_restart_soc(struct restart_handler *rst)
{
	/*
	 * Perform a hardware reset with the use of the Watchdog timer.
	 */
	writel(AT91RM9200_ST_RSTEN | AT91RM9200_ST_EXTEN | 1, st + AT91RM9200_ST_WDMR);
	writel(AT91RM9200_ST_WDRST, st + AT91RM9200_ST_CR);

	/* Not reached */
	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(at91rm9200_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
