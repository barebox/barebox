/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 */

#ifndef _IMX_REGS_H
#define _IMX_REGS_H

/* ------------------------------------------------------------------------
 *  Motorola IMX system registers
 * ------------------------------------------------------------------------
 */

# ifndef __ASSEMBLY__
# define __REG(x)	(*((volatile u32 *)(x)))
# define __REG16(x)     (*(volatile u16 *)(x))
# define __REG2(x,y)    (*(volatile u32 *)((u32)&__REG(x) + (y)))
# else
#  define __REG(x) (x)
#  define __REG16(x) (x)
#  define __REG2(x,y) ((x)+(y))
#endif

#ifdef CONFIG_ARCH_IMX1
# include <mach/imx1-regs.h>
#elif defined CONFIG_ARCH_IMX21
# include <mach/imx21-regs.h>
#elif defined CONFIG_ARCH_IMX27
# include <mach/imx27-regs.h>
#elif defined CONFIG_ARCH_IMX31
# include <mach/imx31-regs.h>
#elif defined CONFIG_ARCH_IMX35
# include <mach/imx35-regs.h>
#elif defined CONFIG_ARCH_IMX25
# include <mach/imx25-regs.h>
#elif defined CONFIG_ARCH_IMX51
# include <mach/imx51-regs.h>
#elif defined CONFIG_ARCH_IMX53
# include <mach/imx53-regs.h>
#elif defined CONFIG_ARCH_IMX6
# include <mach/imx6-regs.h>
#else
# error "unknown i.MX soc type"
#endif

/* There's a off-by-one betweem the gpio bank number and the gpiochip */
/* range e.g. GPIO_1_5 is gpio 5 under linux */
#define IMX_GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

#endif				/* _IMX_REGS_H */
