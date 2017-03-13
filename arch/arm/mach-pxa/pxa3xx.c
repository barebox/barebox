/*
 * (C) Copyright 2015 Robert Jarzmik <robert.jarzmik@free.fr>
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
#include <poweroff.h>
#include <reset_source.h>
#include <mach/hardware.h>
#include <mach/pxa-regs.h>

extern void pxa3xx_suspend(int mode);

static int pxa_detect_reset_source(void)
{
	u32 reg = ARSR;

	/*
	 * Order is important, as many bits can be set together
	 */
	if (reg & ARSR_GPR)
		reset_source_set(RESET_RST);
	else if (reg & ARSR_WDT)
		reset_source_set(RESET_WDG);
	else if (reg & ARSR_HWR)
		reset_source_set(RESET_POR);
	else if (reg & ARSR_LPMR)
		reset_source_set(RESET_WKE);
	else
		reset_source_set(RESET_UKWN);

	return 0;
}

void pxa_clear_reset_source(void)
{
	ARSR = ARSR_GPR | ARSR_LPMR | ARSR_WDT | ARSR_HWR;
}

device_initcall(pxa_detect_reset_source);

static void __noreturn pxa3xx_poweroff(struct poweroff_handler *handler)
{
	shutdown_barebox();

	/* Clear last reset source */
	pxa_clear_reset_source();
	pxa3xx_suspend(PXA3xx_PM_S3D4C4);
	unreachable();
}

static int pxa3xx_init(void)
{
	poweroff_handler_register_fn(pxa3xx_poweroff);

	pxa_detect_reset_source();

	return 0;
}
device_initcall(pxa3xx_init);
