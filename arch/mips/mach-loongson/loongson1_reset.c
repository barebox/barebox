// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov at gmail.com>
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>
#include <mach/loongson1.h>

static void __noreturn longhorn_restart_soc(struct restart_handler *rst)
{
	__raw_writel(0x1, LS1X_WDT_EN);
	__raw_writel(0x1, LS1X_WDT_SET);
	__raw_writel(0x1, LS1X_WDT_TIMER);

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(longhorn_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
