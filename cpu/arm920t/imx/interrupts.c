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
#include <clock.h>
#include <arm920t.h>
#include <asm/arch/imx-regs.h>

uint64_t imx_clocksource_read(void)
{
	return TCN1;
}

static struct clocksource cs = {
	.read	= imx_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 10,
};

int interrupt_init (void)
{
	int i;
	/* setup GP Timer 1 */
	TCTL1 = TCTL_SWR;
	for ( i=0; i<100; i++) TCTL1 = 0; /* We have no udelay by now */
	TPRER1 = 0;
	TCTL1 |= TCTL_FRR | (1<<1); /* Freerun Mode, PERCLK1 input */

	TCTL1 &= ~TCTL_TEN;
	TCTL1 |= TCTL_TEN; /* Enable timer */

        cs.mult = clocksource_hz2mult(get_PERCLK1(), cs.shift);

        init_clock(&cs);

	return 0;
}

/*
 * Reset the cpu by setting up the watchdog timer and let him time out
 */
void reset_cpu (ulong ignored)
{
	/* Disable watchdog and set Time-Out field to 0 */
	WCR = 0x00000000;

	/* Write Service Sequence */
	WSR = 0x00005555;
	WSR = 0x0000AAAA;

	/* Enable watchdog */
	WCR = 0x00000001;

	while (1);
	/*NOTREACHED*/
}
