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

#include <mach/devices.h>

void __iomem *v2m_wdt_base;

static void vexpress_reset_soc(struct restart_handler *rst)
{
	writel(LOAD_MIN, v2m_wdt_base + WDTLOAD);
	writeb(RESET_ENABLE, v2m_wdt_base + WDTCONTROL);

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(vexpress_reset_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
