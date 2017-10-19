/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPL v2
 */
#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <driver.h>
#include <errno.h>
#include <linux/amba/sp804.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <asm/hardware/arm_timer.h>

static __iomem void *sp804_base;
static struct clk *sp804_clk;

static uint64_t sp804_read(void)
{
	return ~readl(sp804_base + TIMER_VALUE);
}

static struct clocksource sp804_clksrc = {
	.read	= sp804_read,
	.shift	= 20,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int sp804_probe(struct amba_device *dev, const struct amba_id *id)
{
	u32 tick_rate;
	int ret;

	if (sp804_base) {
		dev_dbg(&dev->dev, "skipping secondary instance\n");
		return 0;
	}

	sp804_clk = clk_get(&dev->dev, NULL);
	if (IS_ERR(sp804_clk)) {
		ret = PTR_ERR(sp804_clk);
		dev_err(&dev->dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(sp804_clk);
	if (ret < 0) {
		dev_err(&dev->dev, "clock failed to enable: %d\n", ret);
		clk_put(sp804_clk);
		return ret;
	}

	sp804_base = amba_get_mem_region(dev);

	tick_rate = clk_get_rate(sp804_clk);

	/* setup timer 0 as free-running clocksource */
	writel(0, sp804_base + TIMER_CTRL);
	writel(0xffffffff, sp804_base + TIMER_LOAD);
	writel(0xffffffff, sp804_base + TIMER_VALUE);
	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC,
		sp804_base + TIMER_CTRL);

	sp804_clksrc.mult = clocksource_hz2mult(tick_rate, sp804_clksrc.shift);

	return init_clock(&sp804_clksrc);
}

static struct amba_id sp804_ids[] = {
	{
		.id	= AMBA_ARM_SP804_ID,
		.mask	= AMBA_ARM_SP804_ID_MASK,
	},
	{ 0, 0 },
};

struct amba_driver sp804_driver = {
	.drv = {
		.name = "sp804",
	},
	.probe		= sp804_probe,
	.id_table	= sp804_ids,
};

static int sp804_init(void)
{
	return amba_driver_register(&sp804_driver);
}
coredevice_initcall(sp804_init);
