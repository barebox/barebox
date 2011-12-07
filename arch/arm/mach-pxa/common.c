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
#include <asm/io.h>

#define OSMR3	0x40A0000C
#define OSCR	0x40A00010
#define OSSR	0x40A00014
#define OWER	0x40A00018

#define OWER_WME	(1 << 0)	/* Watch-dog Match Enable */
#define OSSR_M3		(1 << 3)	/* Match status channel 3 */

void reset_cpu(ulong addr)
{
	/* Initialize the watchdog and let it fire */
	writel(OWER_WME, OWER);
	writel(OSSR_M3, OSSR);
	writel(readl(OSCR) + 368640, OSMR3);  /* ... in 100 ms */

	while (1);
}
