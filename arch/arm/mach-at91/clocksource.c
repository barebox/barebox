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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/hardware.h>
#include <asm/io.h>

uint64_t at91sam9_clocksource_read(void)
{
	return readl(AT91C_PITC_PIIR);
}

static struct clocksource cs = {
	.read	= at91sam9_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 10,
};

static int clocksource_init (void)
{
	/*
	 * Enable PITC Clock
	 * The clock is already enabled for system controller in boot
	 */
	writel(1 << AT91C_ID_SYS, AT91C_PMC_PCER);

	/* Enable PITC */
	writel(0xfffff | AT91C_PITC_PITEN, AT91C_PITC_PIMR);

	cs.mult = clocksource_hz2mult(1000000 * 6, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);

/*
 * Reset the cpu by setting up the watchdog timer and let it time out
 */
void reset_cpu (ulong ignored)
{
	writel((0xa5 << 24) | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST,
			AT91C_RSTC_RCR);
}
EXPORT_SYMBOL(reset_cpu);
