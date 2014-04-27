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

#include <asm/io.h>
#include <common.h>
#include <mach/rockchip-regs.h>

void __noreturn reset_cpu(unsigned long addr)
{
	/* Map bootrom from address 0 */
	writel(RK_SOC_CON0_REMAP << 16, RK_GRF_BASE + RK_GRF_SOC_CON0);
	/* Reset */
	writel(0xeca8, RK_CRU_BASE + RK_CRU_GLB_SRST_SND);

	while (1)
		;
}
EXPORT_SYMBOL(reset_cpu);
