/*
 * (C) Copyright 2009 Sascha Hauer <s.hauer@pengutronix.de>
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
 */

#include <common.h>
#include <of.h>
#include <init.h>
#include <restart.h>
#include <mach/pxa/pxa-regs.h>
#include <asm/io.h>

#define OSMR3	0x40A0000C
#define OSCR	0x40A00010
#define OSSR	0x40A00014
#define OWER	0x40A00018

#define OWER_WME	(1 << 0)	/* Watch-dog Match Enable */
#define OSSR_M3		(1 << 3)	/* Match status channel 3 */

/*
 * Where to park the counter, and how far below the match to park it. The
 * counter runs at 3.25MHz on PXA3xx, so 16 ticks is around 5us.
 */
#define RESET_MATCH	0x1000
#define RESET_MARGIN	16

static void __noreturn pxa_restart_soc(struct restart_handler *rst,
				       unsigned long flags)
{
	/* Clear last reset source */
	pxa_clear_reset_source();

	/* Initialize the watchdog and let it fire */
	writel(OWER_WME, OWER);
	writel(OSSR_M3, OSSR);

	/*
	 * Set the match, then put the counter just below it. Both writes are
	 * absolute, so the match is still ahead of the counter whenever the
	 * second one lands - unlike computing the match from the counter,
	 * which has to leave enough margin for its own store to get there and
	 * misses a whole 32bit wrap of the counter if it does not.
	 */
	writel(RESET_MATCH, OSMR3);
	writel(RESET_MATCH - RESET_MARGIN, OSCR);

	/*
	 * A few microseconds out. Deliberately not hang(), which complains
	 * about a board needing a reset while it is being reset.
	 */
	for (;;)
		;
}

static int restart_register_feature(void)
{
	if (!of_machine_is_compatible("marvell,pxa3xx"))
		return 0;

	restart_handler_register_fn("soc-wdt", pxa_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
