/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <io.h>
#include <linux/amba/sp805.h>

#include <mach/devices.h>

void __iomem *v2m_wdt_base;

void reset_cpu(ulong addr)
{
	writel(LOAD_MIN, v2m_wdt_base + WDTLOAD);
	writeb(RESET_ENABLE, v2m_wdt_base + WDTCONTROL);

	while (1)
		;
}
