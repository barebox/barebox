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
#include <init.h>
#include <restart.h>
#include <mach/rockchip/rk3188-regs.h>
#include <mach/rockchip/rockchip.h>

static void __noreturn rockchip_restart_soc(struct restart_handler *rst,
					    unsigned long flags)
{
	/* Map bootrom from address 0 */
	writel(RK_SOC_CON0_REMAP << 16, RK_GRF_BASE + RK_GRF_SOC_CON0);
	/* Reset */
	writel(0xeca8, RK_CRU_BASE + RK_CRU_GLB_SRST_SND);

	hang();
}

int rk3188_init(void)
{
	restart_handler_register_fn("soc", rockchip_restart_soc);

	return 0;
}
