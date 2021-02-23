/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2007 MontaVista Software, Inc. */

/*
 * Local header file for DaVinci time code.
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 */
#ifndef __ARCH_ARM_MACH_DAVINCI_TIME_H
#define __ARCH_ARM_MACH_DAVINCI_TIME_H

#include <mach/hardware.h>

#define DAVINCI_TIMER0_BASE		(IO_PHYS + 0x21400)
#define DAVINCI_TIMER1_BASE		(IO_PHYS + 0x21800)
#define DAVINCI_WDOG_BASE		(IO_PHYS + 0x21C00)

#endif /* __ARCH_ARM_MACH_DAVINCI_TIME_H */
