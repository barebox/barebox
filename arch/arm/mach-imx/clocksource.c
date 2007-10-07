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
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>

#define GPT(x) __REG(IMX_TIM1_BASE + (x))

uint64_t imx_clocksource_read(void)
{
	return GPT(GPT_TCN);
}

static struct clocksource cs = {
	.read	= imx_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 10,
};

static int clocksource_init (void)
{
	int i;
	/* setup GP Timer 1 */
	GPT(GPT_TCTL) = TCTL_SWR;

	for (i = 0; i < 100; i++)
		GPT(GPT_TCTL) = 0; /* We have no udelay by now */

	GPT(GPT_TPRER) = 0;
	GPT(GPT_TCTL) |= TCTL_FRR | (1<<1); /* Freerun Mode, PERCLK1 input */

	GPT(GPT_TCTL) &= ~TCTL_TEN;
	GPT(GPT_TCTL) |= TCTL_TEN; /* Enable timer */

	cs.mult = clocksource_hz2mult(imx_get_perclk1(), cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);

/*
 * Reset the cpu by setting up the watchdog timer and let it time out
 */
void reset_cpu (ulong ignored)
{
	/* Disable watchdog and set Time-Out field to 0 */
	WCR = 0x00000000;

	/* Write Service Sequence */
	WSR = 0x00005555;
	WSR = 0x0000AAAA;

	/* Enable watchdog */
#ifdef CONFIG_ARCH_IMX1
	WCR = 0x00000001;
#elif defined CONFIG_ARCH_IMX27
	WCR = 1 << 2;
#else
#error unknown i.MX soc
#endif

	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);
