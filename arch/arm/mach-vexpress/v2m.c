/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <io.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/amba/bus.h>

#include <asm/hardware/arm_timer.h>
#include <asm/hardware/sp810.h>

#include <mach/devices.h>

void __iomem *v2m_sysreg_base;

static void v2m_sysctl_init(void __iomem *base)
{
	u32 scctrl;

	if (WARN_ON(!base))
		return;

	/* Select 1MHz TIMCLK as the reference clock for SP804 timers */
	scctrl = readl(base + SCCTRL);
	scctrl |= SCCTRL_TIMEREN0SEL_TIMCLK;
	scctrl |= SCCTRL_TIMEREN1SEL_TIMCLK;
	writel(scctrl, base + SCCTRL);
}

void vexpress_a9_legacy_init(void)
{
	v2m_wdt_base = IOMEM(0x1000f000);
	v2m_sysreg_base = IOMEM(0x10001000);
	v2m_sysctl_init(IOMEM(0x10001000));
}

void vexpress_init(void)
{
	v2m_wdt_base = IOMEM(0x1c0f0000);
	v2m_sysreg_base = IOMEM(0x1c020000);
	v2m_sysctl_init(IOMEM(0x1c020000));
}
