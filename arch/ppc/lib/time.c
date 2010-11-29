/*
 * (C) Copyright 2000, 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <clock.h>
#include <init.h>
#include <mach/clocks.h>
#include <asm/common.h>

/* ------------------------------------------------------------------------- */

static int init_timebase (void)
{
#if defined(CONFIG_5xx) || defined(CONFIG_8xx)
	volatile immap_t *immap = (immap_t *) CFG_IMMR;

	/* unlock */
	immap->im_sitk.sitk_tbk = KAPWR_KEY;
#endif

	/* reset */
	asm ("li 3,0 ; mttbu 3 ; mttbl 3 ;");

#if defined(CONFIG_5xx) || defined(CONFIG_8xx)
	/* enable */
	immap->im_sit.sit_tbscr |= TBSCR_TBE;
#endif
	return (0);
}
/* ------------------------------------------------------------------------- */

uint64_t ppc_clocksource_read(void)
{
	return get_ticks();
}

static struct clocksource cs = {
	.read	= ppc_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 15,
};

static int clocksource_init (void)
{
	init_timebase();

	cs.mult = clocksource_hz2mult(get_timebase_clock(), cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
