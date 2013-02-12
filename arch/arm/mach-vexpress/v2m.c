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

static const char *v2m_osc2_periphs[] = {
	"mb:uart0", "uart-pl0110",	/* PL011 UART0 */
	"mb:uart1", "uart-pl0111",	/* PL011 UART1 */
	"mb:uart2", "uart-pl0112",	/* PL011 UART2 */
	"mb:uart3", "uart-pl0113",	/* PL011 UART3 */
};

static void v2m_clk_init(void)
{
	struct clk *clk;
	int i;

	clk = clk_fixed("dummy_apb_pclk", 0);
	clk_register_clkdev(clk, "apb_pclk", NULL);

	clk = clk_fixed("mb:sp804_clk", 1000000);
	clk_register_clkdev(clk, NULL, "sp804");

	clk = clk_fixed("mb:osc2", 24000000);
	for (i = 0; i < ARRAY_SIZE(v2m_osc2_periphs); i++)
		clk_register_clkdev(clk, NULL, v2m_osc2_periphs[i]);

}

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

static void __init v2m_sp804_init(void __iomem *base)
{
	writel(0, base + TIMER_1_BASE + TIMER_CTRL);

	amba_apb_device_add(NULL, "sp804", DEVICE_ID_SINGLE, (resource_size_t)base, 4096, NULL, 0);
}

void vexpress_a9_legacy_init(void)
{
	v2m_wdt_base = IOMEM(0x1000f000);
	v2m_sysreg_base = IOMEM(0x10001000);
	v2m_sysctl_init(IOMEM(0x10001000));
	v2m_clk_init();

	v2m_sp804_init(IOMEM(0x10011000));
}

void vexpress_init(void)
{
	v2m_wdt_base = IOMEM(0x1c0f0000);
	v2m_sysreg_base = IOMEM(0x1c020000);
	v2m_sysctl_init(IOMEM(0x1c020000));
	v2m_clk_init();

	v2m_sp804_init(IOMEM(0x1c110000));
}
