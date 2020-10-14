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
#include <linux/clk.h>
#include <linux/err.h>

#define TWD_TIMER_LOAD			0x00
#define TWD_TIMER_COUNTER		0x04
#define TWD_TIMER_CONTROL		0x08
#define TWD_TIMER_INTSTAT		0x0C

#define TWD_TIMER_CONTROL_ENABLE	(1 << 0)
#define TWD_TIMER_CONTROL_ONESHOT	(0 << 1)
#define TWD_TIMER_CONTROL_PERIODIC	(1 << 1)
#define TWD_TIMER_CONTROL_IT_ENABLE	(1 << 2)

#define TWD_TIMER_CONTROL_PRESC(x)	(((x) & 0xff) << 8)

static __iomem void *twd_base;
static struct clk *twd_clk;

static uint64_t smp_twd_read(void)
{
	return ~readl(twd_base + TWD_TIMER_COUNTER);
}

static struct clocksource smp_twd_clksrc = {
	.read	= smp_twd_read,
	.shift	= 20,
	.mask	= CLOCKSOURCE_MASK(32),
};

#define SMP_TWD_MAX_FREQ (25 *1000 * 1000)

static int smp_twd_probe(struct device_d *dev)
{
	struct resource *iores;
	u32 tick_rate;
	u32 val;
	int ret;
	u32 presc = 0;

	twd_clk = clk_get(dev, NULL);
	if (IS_ERR(twd_clk)) {
		ret = PTR_ERR(twd_clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(twd_clk);
	if (ret < 0) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		clk_put(twd_clk);
		return ret;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	twd_base = IOMEM(iores->start);

	tick_rate = clk_get_rate(twd_clk);
	if (tick_rate > SMP_TWD_MAX_FREQ) {
		presc = tick_rate / SMP_TWD_MAX_FREQ;
		if (presc)
			presc--;
		presc = min((u32)0xff, presc);
		tick_rate /= presc + 1;
	}

	val = TWD_TIMER_CONTROL_PRESC(presc) |
	      TWD_TIMER_CONTROL_PERIODIC;
	writel(val, twd_base + TWD_TIMER_CONTROL);

	writel(0xffffffff, twd_base + TWD_TIMER_LOAD);

	val = readl(twd_base + TWD_TIMER_CONTROL);
	val |= TWD_TIMER_CONTROL_ENABLE;
	writel(val, twd_base + TWD_TIMER_CONTROL);

	smp_twd_clksrc.mult = clocksource_hz2mult(tick_rate, smp_twd_clksrc.shift);

	return init_clock(&smp_twd_clksrc);
}

static __maybe_unused struct of_device_id smp_twd_compatible[] = {
	{
		.compatible = "arm,cortex-a9-twd-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d smp_twd_driver = {
	.name = "smp_twd",
	.probe = smp_twd_probe,
	.of_compatible = DRV_OF_COMPAT(smp_twd_compatible),
};

coredevice_platform_driver(smp_twd_driver);
