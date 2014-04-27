/*
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_ROCKCHIP_PLL_H
#define __MACH_ROCKCHIP_PLL_H

enum rk3188_plls {
	RK3188_APLL = 0,	/* ARM */
	RK3188_DPLL,		/* DDR */
	RK3188_CPLL,		/* Codec */
	RK3188_GPLL,		/* General */
};

int rk3188_pll_set_parameters(int pll, int nr, int nf, int no);

#endif /* __MACH_ROCKCHIP_PLL_H */
