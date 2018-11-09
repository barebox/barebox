// SPDX-License-Identifier: GPL-2.0

/*
 * Clocksource driver for generic Cortex A9 timer block
 *
 * Copyright (C) 2018 Zodiac Inflight Innovations
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * based on corresponding driver from Linux kernel with the following
 * copyright:
 *
 * drivers/clocksource/arm_global_timer.c
 *
 * Copyright (C) 2013 STMicroelectronics (R&D) Limited.
 * Author: Stuart Menefy <stuart.menefy@st.com>
 * Author: Srinivas Kandagatla <srinivas.kandagatla@st.com>
 */
#include <common.h>
#include <init.h>
#include <clock.h>
#include <linux/clk.h>
#include <io.h>
#include <asm/system.h>

#define GT_COUNTER0	0x00
#define GT_COUNTER1	0x04

#define GT_CONTROL	0x08
#define GT_CONTROL_TIMER_ENABLE		BIT(0)  /* this bit is NOT banked */

static void __iomem *gt_base;

/*
 * To get the value from the Global Timer Counter register proceed as follows:
 * 1. Read the upper 32-bit timer counter register
 * 2. Read the lower 32-bit timer counter register
 * 3. Read the upper 32-bit timer counter register again. If the value is
 *  different to the 32-bit upper value read previously, go back to step 2.
 *  Otherwise the 64-bit timer counter value is correct.
 */
static uint64_t arm_global_clocksource_read(void)
{
	uint64_t counter;
	uint32_t lower;
	uint32_t upper, old_upper;

	upper = readl(gt_base + GT_COUNTER1);
	do {
		old_upper = upper;
		lower = readl(gt_base + GT_COUNTER0);
		upper = readl(gt_base + GT_COUNTER1);
	} while (upper != old_upper);

	counter = upper;
	counter <<= 32;
	counter |= lower;
	return counter;
}

static struct clocksource cs = {
	.read	= arm_global_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
};

static int arm_global_timer_probe(struct device_d *dev)
{
	struct resource *iores;
	struct clk *clk;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(clk);
	if (ret) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		return ret;
	}

	gt_base = IOMEM(iores->start);

	cs.mult = clocksource_hz2mult(clk_get_rate(clk), cs.shift);

	writel(0, gt_base + GT_CONTROL);
	writel(0, gt_base + GT_COUNTER0);
	writel(0, gt_base + GT_COUNTER1);
	/* enables timer on all the cores */
	writel(GT_CONTROL_TIMER_ENABLE, gt_base + GT_CONTROL);

	return init_clock(&cs);
}

static struct of_device_id arm_global_timer_dt_ids[] = {
	{ .compatible = "arm,cortex-a9-global-timer", },
	{ }
};

static struct driver_d arm_global_timer_driver = {
	.name = "arm-global-timer",
	.probe = arm_global_timer_probe,
	.of_compatible = DRV_OF_COMPAT(arm_global_timer_dt_ids),
};
postcore_platform_driver(arm_global_timer_driver);

