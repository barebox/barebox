/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief Resetting an malta board
 */

#include <common.h>
#include <asm/io.h>
#include <mach/iomap.h>

#define PRM_RSTCTRL		TEGRA_PMC_BASE

void __noreturn reset_cpu(ulong addr)
{
	int rstctrl;

	rstctrl = __raw_readl((char *)PRM_RSTCTRL);
	rstctrl |= 0x10;
	__raw_writel(rstctrl, (char *)PRM_RSTCTRL);

	unreachable();
}
EXPORT_SYMBOL(reset_cpu);
