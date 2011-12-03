/*
 * clock.h - implementation of the PXA clock functions
 *
 * Copyright (C) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <mach/clock.h>
#include <mach/pxa-regs.h>

/* Crystal clock: 13MHz */
#define BASE_CLK	13000000

unsigned long pxa_get_uartclk(void)
{
	return 14857000;
}
