/*
 * (c) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * Copyright (C) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

#ifdef CONFIG_ARCH_PXA2XX
#define cpu_is_pxa2xx()	(1)
#else
#define cpi_is_pxa2xx()	(0)
#endif

#ifdef CONFIG_ARCH_PXA25X
#define cpu_is_pxa25x()	(1)
#else
#define cpu_is_pxa25x()	(0)
#endif

#ifdef CONFIG_ARCH_PXA27X
#define cpu_is_pxa27x()	(1)
#else
#define cpu_is_pxa27x()	(0)
#endif

#endif	/* !__MACH_HARDWARE_H */
