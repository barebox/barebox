/*
 * clock.h - implementation of the PXA clock functions
 *
 * Copyright (C) 2014 by Robert Jarzmik <robert.jarzmik@free.fr>
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

unsigned long pxa_get_pwmclk(void)
{
	return BASE_CLK;
}

unsigned long pxa_get_nandclk(void)
{
	if (cpu_is_pxa320())
		return 104000000;
	else
		return 156000000;
}
