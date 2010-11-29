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
#include <mach/at91_pit.h>
#include <mach/at91_pmc.h>
#include <mach/at91_rstc.h>
#include <mach/io.h>
#include <asm/io.h>

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

/*
 * Reset the cpu through the reset controller
 */
void __noreturn reset_cpu (unsigned long addr)
{
	at91_sys_write(AT91_RSTC_CR, AT91_RSTC_KEY |
				     AT91_RSTC_PROCRST |
				     AT91_RSTC_PERRST);

	/* Not reached */
	while (1);
}
EXPORT_SYMBOL(reset_cpu);
