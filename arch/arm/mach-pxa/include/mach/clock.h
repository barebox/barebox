
/*
 * clock.h - definitions of the PXA clock functions
 *
 * Copyright (C) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __MACH_CLOCK_H
#define __MACH_CLOCK_H

unsigned long pxa_get_uartclk(void);
unsigned long pxa_get_mmcclk(void);
unsigned long pxa_get_lcdclk(void);
unsigned long pxa_get_pwmclk(void);

#endif	/* !__MACH_CLOCK_H */
