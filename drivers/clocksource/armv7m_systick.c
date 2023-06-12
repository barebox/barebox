// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) Maxime Coquelin 2015
 * Author:  Maxime Coquelin <mcoquelin.stm32@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>

#define SYST_CSR	0x00
#define SYST_RVR	0x04
#define SYST_CVR	0x08
#define SYST_CALIB	0x0c

#define SYSTICK_CTRL_EN		BIT(0)
/* Clock source: 0 = Ref clock, 1 = CPU clock */
#define SYSTICK_CTRL_CPU_CLK	BIT(2)
#define SYSTICK_CAL_NOREF	BIT(31)

#define SYSTICK_LOAD_RELOAD_MASK 0x00FFFFFF

static __iomem void *systick_base;

static u64 armv7m_systick_clocksource_read(void)
{
        return SYSTICK_LOAD_RELOAD_MASK - readl(systick_base + SYST_CVR);
}

static struct clocksource cs = {
	.read	= armv7m_systick_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(24),
	.shift	= 0,
	.priority = 70,
};

static int armv7m_systick_probe(struct device *dev)
{
	struct clk *clk = NULL;
	u32 rate, cal;
	int ret;

	systick_base = dev_request_mem_region_err_null(dev, 0);
	if (!systick_base)
		return -ENOENT;

	ret = of_property_read_u32(dev->of_node, "clock-frequency", &rate);
	if (ret) {
		clk = clk_get(dev, NULL);
		if (IS_ERR(clk))
			return PTR_ERR(clk);

		ret = clk_enable(clk);
		if (ret)
			return ret;

		rate = clk_get_rate(clk);
		if (!rate)
			return -EINVAL;
	}

	writel_relaxed(SYSTICK_LOAD_RELOAD_MASK, systick_base + SYST_RVR);

	cal = readl(systick_base + SYST_CALIB);
	if (cal & SYSTICK_CAL_NOREF)
		writel(SYSTICK_CTRL_EN | SYSTICK_CTRL_CPU_CLK, systick_base + SYST_CSR);
	else
		writel(SYSTICK_CTRL_EN, systick_base + SYST_CSR);

	cs.mult = clocksource_hz2mult(rate, cs.shift);

	return init_clock(&cs);
}

static struct of_device_id armv7m_systick_dt_ids[] = {
	{ .compatible = "arm,armv7m-systick", },
	{ }
};
MODULE_DEVICE_TABLE(of, armv7m_systick_dt_ids);

static struct driver armv7m_systick_driver = {
	.name = "armv7m-systick-timer",
	.probe = armv7m_systick_probe,
	.of_compatible = DRV_OF_COMPAT(armv7m_systick_dt_ids),
};
postcore_platform_driver(armv7m_systick_driver);
