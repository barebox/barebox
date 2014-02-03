/*
 * (C) Copyright 2009 Sascha Hauer <s.hauer@pengutronix.de>
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
 */

#include <common.h>
#include <mach/pxa-regs.h>
#include <asm/io.h>

#define OSMR3	0x40A0000C
#define OSCR	0x40A00010
#define OSSR	0x40A00014
#define OWER	0x40A00018

#define OWER_WME	(1 << 0)	/* Watch-dog Match Enable */
#define OSSR_M3		(1 << 3)	/* Match status channel 3 */

extern void pxa_suspend(int mode);

void reset_cpu(ulong addr)
{
	/* Clear last reset source */
	RCSR = RCSR_GPR | RCSR_SMR | RCSR_WDR | RCSR_HWR;

	/* Initialize the watchdog and let it fire */
	writel(OWER_WME, OWER);
	writel(OSSR_M3, OSSR);
	writel(readl(OSCR) + 368640, OSMR3);  /* ... in 100 ms */

	while (1);
}

void __noreturn poweroff()
{
	shutdown_barebox();

	/* Clear last reset source */
	RCSR = RCSR_GPR | RCSR_SMR | RCSR_WDR | RCSR_HWR;

	pxa_suspend(PWRMODE_DEEPSLEEP);
	unreachable();
}
