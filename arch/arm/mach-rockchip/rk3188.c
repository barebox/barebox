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
#include <mach/rk3188-regs.h>

static void __noreturn rockchip_restart_soc(struct restart_handler *rst)
{
	/* Map bootrom from address 0 */
	writel(RK_SOC_CON0_REMAP << 16, RK_GRF_BASE + RK_GRF_SOC_CON0);
	/* Reset */
	writel(0xeca8, RK_CRU_BASE + RK_CRU_GLB_SRST_SND);

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(rockchip_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
