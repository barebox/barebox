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

unsigned long pxa_get_mmcclk(void)
{
	return 19500000;
}

/*
 * Return the current LCD clock frequency in units of 10kHz as
 */
static unsigned int pxa_get_lcdclk_10khz(void)
{
	unsigned long ccsr;
	unsigned int l, L, k, K;

	ccsr = CCSR;

	l = ccsr & 0x1f;
	k = (l <= 7) ? 1 : (l <= 16) ? 2 : 4;

	L = l * BASE_CLK;
	K = L / k;

	return (K / 10000);
}

unsigned long pxa_get_lcdclk(void)
{
	return pxa_get_lcdclk_10khz() * 10000;
}

unsigned long pxa_get_pwmclk(void)
{
	return BASE_CLK;
}
