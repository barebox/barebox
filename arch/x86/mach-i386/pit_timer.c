/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

/**
 * @file
 * @brief Clocksource based on the 'Programmable Interval Timer' PIT (8253)
 *
 * This timer should be available on almost all PCs. It also should be run
 * at a fixed frequency (1193181.8181 Hz) and not modified to use another
 * reload value than 0xFFFF. So, it always counts from 0xffff down to 0.
 *
 * @note: We can't reprogram the PIT, it will be still used by the BIOS. This
 * clocksource driver does not touch any PIT settings.
 */

#include <init.h>
#include <clock.h>
#include <io.h>

/** base address of the PIT in a standard PC */
#define PIT 0x40

static uint64_t pit_clocksource_read(void)
{
	uint16_t val1, val2;

	outb(0x00, PIT + 3);	/* latch counter 0 */
	outb(0x00, 0x80);

	val1 = inb(PIT);
	outb(0x00, 0x80);

	val2 = inb(PIT);
	val2 <<= 8;

	/* note: its a down counter */
	return 0xFFFFU - (val1 | val2);
}

static struct clocksource cs = {
	.read	= pit_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(16),
	.shift	= 10,
};

static int clocksource_init (void)
{
	cs.mult = clocksource_hz2mult(1193182, cs.shift);
	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
