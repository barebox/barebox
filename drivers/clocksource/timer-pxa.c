// SPDX-License-Identifier: GPL-2.0-only
/*
 * Clocksource and watchdog for the PXA OS timer.
 *
 * The two are one block: match register 3 doubles as the watchdog, so both
 * live in the same driver rather than fighting over the device tree node.
 *
 * (C) Copyright 2009 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <clock.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <watchdog.h>
#include <linux/clk.h>
#include <linux/err.h>

#define OSMR3		0x0c	/* match register 3, the watchdog match */
#define OSCR		0x10	/* counter, free running and running out of reset */
#define OSSR		0x14	/* status */
#define OWER		0x18	/* watchdog enable */

#define OSSR_M3		(1 << 3)	/* match status channel 3 */
#define OWER_WME	(1 << 0)	/* watchdog match enable */

struct pxa_timer {
	void __iomem *base;
	unsigned long rate;
	struct watchdog wd;
};

static inline struct pxa_timer *to_pxa_timer(struct watchdog *wd)
{
	return container_of(wd, struct pxa_timer, wd);
}

static void __iomem *pxa_timer_base;

static uint64_t pxa_clocksource_read(void)
{
	return readl(pxa_timer_base + OSCR);
}

static struct clocksource pxa_cs = {
	.read     = pxa_clocksource_read,
	.mask     = CLOCKSOURCE_MASK(32),
	.shift    = 20,
	.priority = 80,
};

static int pxa_wdt_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct pxa_timer *timer = to_pxa_timer(wd);

	if (!timeout) {
		/*
		 * OWER_WME only ever reads back the way it was written once:
		 * the watchdog cannot be stopped again short of the reset it
		 * is about to cause. Refuse rather than pretend.
		 */
		if (wd->running == WDOG_HW_RUNNING)
			return -ENOSYS;

		return 0;
	}

	writel(readl(timer->base + OSCR) + (u64)timeout * timer->rate,
	       timer->base + OSMR3);
	writel(OSSR_M3, timer->base + OSSR);
	writel(OWER_WME, timer->base + OWER);

	wd->running = WDOG_HW_RUNNING;

	return 0;
}

static int pxa_timer_probe(struct device *dev)
{
	struct resource *iores;
	struct pxa_timer *timer;
	struct clk *clk;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "cannot get clock\n");

	ret = clk_enable(clk);
	if (ret)
		return dev_err_probe(dev, ret, "cannot enable clock\n");

	timer = xzalloc(sizeof(*timer));
	timer->base = IOMEM(iores->start);
	timer->rate = clk_get_rate(clk);
	if (!timer->rate)
		return dev_err_probe(dev, -EINVAL, "clock has no rate\n");

	pxa_timer_base = timer->base;
	pxa_cs.mult = clocksource_hz2mult(timer->rate, pxa_cs.shift);

	ret = init_clock(&pxa_cs);
	if (ret)
		return ret;

	timer->wd.set_timeout = pxa_wdt_set_timeout;
	timer->wd.hwdev = dev;
	timer->wd.name = "pxa-wdt";
	/* the counter is 32 bit, so that is as far ahead as a match reaches */
	timer->wd.timeout_max = U32_MAX / timer->rate;
	timer->wd.running = readl(timer->base + OWER) & OWER_WME ?
			WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;

	ret = watchdog_register(&timer->wd);
	if (ret)
		dev_warn(dev, "failed to register watchdog: %pe\n",
			 ERR_PTR(ret));

	return 0;
}

static const struct of_device_id pxa_timer_dt_ids[] = {
	{ .compatible = "marvell,pxa-timer" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pxa_timer_dt_ids);

static struct driver pxa_timer_driver = {
	.name = "pxa-timer",
	.probe = pxa_timer_probe,
	.of_compatible = pxa_timer_dt_ids,
};
postcore_platform_driver(pxa_timer_driver);

/*
 * Under deep probe nothing refers to the timer by phandle, so it is not probed
 * until the device tree walk reaches it - and that walk is in device tree
 * order, which on PXA3xx puts the NAND controller a long way ahead of the
 * timer. Everything in between would run its timeouts against the dummy
 * clocksource.
 */
static int pxa_timer_of_init(void)
{
	return of_devices_ensure_probed_by_dev_id(pxa_timer_dt_ids);
}
coredevice_initcall(pxa_timer_of_init);
