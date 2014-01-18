/*
 * (C) Copyright 2014 Robert Jarzmik <robert.jarzmik@free.fr>
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
#include <init.h>
#include <reset_source.h>
#include <mach/pxa-regs.h>

static int pxa_detect_reset_source(void)
{
	u32 reg = RCSR;

	/*
	 * Order is important, as many bits can be set together
	 */
	if (reg & RCSR_GPR)
		set_reset_source(RESET_RST);
	else if (reg & RCSR_WDR)
		set_reset_source(RESET_WDG);
	else if (reg & RCSR_HWR)
		set_reset_source(RESET_POR);
	else if (reg & RCSR_SMR)
		set_reset_source(RESET_WKE);
	else
		set_reset_source(RESET_UKWN);

	return 0;
}

device_initcall(pxa_detect_reset_source);
