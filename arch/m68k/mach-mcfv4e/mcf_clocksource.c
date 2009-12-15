/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Implements the clocksource for Coldfire V4E
 */
#include <common.h>
#include <init.h>
#include <clock.h>
#include <mach/mcf54xx-regs.h>
#include <mach/clocks.h>
#include <proc/processor.h> //FIXME - move to other file

#ifdef CONFIG_USE_IRQ

static uint32_t mcf_sltirq_hits; // FIXME: test code

static int slt_timer_default_isr(void *not_used, void *t)
{
	struct mcf5xxx_slt *slt = MCF_SLT_Address(0);
	if ( MCF_INTC_IPRH | MCF_INTC_IPRH_INT54 )
	{
		if (slt->SSR & MCF_SLT_SSR_ST)
		{
			slt->SSR = 0UL
				| MCF_SLT_SSR_ST;
		}
		mcf_sltirq_hits++;
		return 1;
	}
	return 0;
}
#endif


static uint64_t mcf_clocksource_read(void)
{
	struct mcf5xxx_slt *slt = MCF_SLT_Address(0);
	uint32_t sltcnt;
	uint64_t rc = 0;

	if (slt->SSR & MCF_SLT_SSR_ST)
	{
#ifndef CONFIG_USE_IRQ
		slt->SSR = 0UL
			| MCF_SLT_SSR_ST;
#endif

		rc = 0xffffffffULL + 1;
	}

	sltcnt = 0xffffffffUL - slt->SCNT;

	rc += sltcnt;

	return rc;
}


static struct clocksource cs = {
	.read	= mcf_clocksource_read,
	.mask	= 0xffffffff,
	.mult   = 1,
	.shift	= 0,
};

static int clocksource_init (void)
{
	struct mcf5xxx_slt *slt = MCF_SLT_Address(0);
	uint32_t sltclk = mcfv4e_get_bus_clk() * 1000000;

	/* Setup a slice timer terminal count */
	slt->STCNT = 0xffffffff;

	/* Reset status bits */
        slt->SSR = 0UL
		| MCF_SLT_SSR_ST
		| MCF_SLT_SSR_BE;

        /* Start timer to run continously */
	slt->SCR   = 0UL
		| MCF_SLT_SCR_TEN
		| MCF_SLT_SCR_RUN
		| MCF_SLT_SCR_IEN;	// FIXME - add irq handler

        /* Setup up multiplier */
	cs.mult = clocksource_hz2mult(sltclk, cs.shift);

	/*
	* Register the timer interrupt handler
	*/
	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);

#ifdef CONFIG_USE_IRQ

static int clocksource_irq_init(void)
{
	if (!mcf_interrupts_register_handler(
	     118, // FIXME!
	     (int (*)(void*,void*))slt_timer_default_isr,
	     NULL,
	     NULL)
	              )
	{
		return 1;
	}
	// Setup ICR
	MCF_INTC_ICR54 = MCF_INTC_ICRn_IL(1) | MCF_INTC_ICRn_IP(0);
	// Enable IRQ source
	MCF_INTC_IMRH &= ~MCF_INTC_IMRH_INT_MASK54;
	return 0;
}

postcore_initcall(clocksource_irq_init);

#endif
