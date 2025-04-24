/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>
#include <linux/amba/sp805.h>
#include <mach/vexpress/vexpress.h>
#include <mach/vexpress/devices.h>

void __iomem *v2m_wdt_base;

static void __noreturn vexpress_reset_soc(struct restart_handler *rst,
					  unsigned long flags)
{
	writel(LOAD_MIN, v2m_wdt_base + WDTLOAD);
	writeb(RESET_ENABLE, v2m_wdt_base + WDTCONTROL);

	hang();
}

void vexpress_restart_register_feature(void __iomem *base)
{
	v2m_wdt_base = base;

	restart_handler_register_fn("soc-wdt", vexpress_reset_soc);
}
