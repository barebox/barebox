/*
 *  mach-nomadik/include/mach/system.h
 *
 *  Copyright (C) 2008 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <mach/hardware.h>

static void __noreturn nomadik_restart_soc(struct restart_handler *rst)
{
	void __iomem *src_rstsr = (void *)(NOMADIK_SRC_BASE + 0x18);

	/* FIXME: use egpio when implemented */

	/* Write anything to Reset status register */
	writel(1, src_rstsr);

	/* Not reached */
	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(nomadik_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
