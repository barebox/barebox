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
#include <poweroff.h>
#include <reset_source.h>
#include <mach/hardware.h>
#include <mach/pxa-regs.h>

extern void pxa_suspend(int mode);

static int pxa_detect_reset_source(void)
{
	u32 reg = RCSR;

	/*
	 * Order is important, as many bits can be set together
	 */
	if (reg & RCSR_GPR)
		reset_source_set(RESET_RST);
	else if (reg & RCSR_WDR)
		reset_source_set(RESET_WDG);
	else if (reg & RCSR_HWR)
		reset_source_set(RESET_POR);
	else if (reg & RCSR_SMR)
		reset_source_set(RESET_WKE);
	else
		reset_source_set(RESET_UKWN);

	return 0;
}

void pxa_clear_reset_source(void)
{
	RCSR = RCSR_GPR | RCSR_SMR | RCSR_WDR | RCSR_HWR;
}

static void __noreturn pxa2xx_poweroff(struct poweroff_handler *handler)
{
	shutdown_barebox();

	/* Clear last reset source */
	pxa_clear_reset_source();
	pxa_suspend(PWRMODE_DEEPSLEEP);
	unreachable();
}

static int pxa2xx_init(void)
{
	poweroff_handler_register_fn(pxa2xx_poweroff);

	pxa_detect_reset_source();

	return 0;
}
device_initcall(pxa2xx_init);
