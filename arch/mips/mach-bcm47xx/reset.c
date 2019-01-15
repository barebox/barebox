// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>
#include <mach/hardware.h>

static void __noreturn bcm47xx_restart_soc(struct restart_handler *rst)
{
	__raw_writel(GORESET, (char *)SOFTRES_REG);

	hang();
	/*NOTREACHED*/
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(bcm47xx_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
