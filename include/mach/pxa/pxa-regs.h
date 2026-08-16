/*
 * (c) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * Copyright (C) 2010 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __MACH_PXA_REGS_H
#define __MACH_PXA_REGS_H

#ifndef __ASSEMBLY__
# define __REG(x)	(*((volatile u32 *)(x)))
# define __REG16(x)	(*(volatile u16 *)(x))
# define __REG2(x, y)	(*(volatile u32 *)((u32)&__REG(x) + (y)))
#else
# define __REG(x)	(x)
# define __REG16(x)	(x)
# define __REG2(x, y)	((x) + (y))
#endif

#include <mach/pxa/pxa3xx-regs.h>

#endif	/* !__MACH_PXA_REGS_H */
