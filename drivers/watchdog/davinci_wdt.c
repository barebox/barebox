/*
 * drivers/char/watchdog/davinci_wdt.c
 *
 * Watchdog driver for DaVinci DM644x/DM646x processors
 *
 * Copyright (C) 2006-2013 Texas Instruments.
 * Copyright (C) 2015 Jan Luebbe <jluebbe@debian.org>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>

#include <linux/clk.h>

/* Timer register set definition */
#define PID12	(0x0)
#define EMUMGT	(0x4)
#define TIM12	(0x10)
#define TIM34	(0x14)
#define PRD12	(0x18)
#define PRD34	(0x1C)
#define TCR	(0x20)
#define TGCR	(0x24)
#define WDTCR	(0x28)

/* TCR bit definitions */
#define ENAMODE12_DISABLED	(0 << 6)
#define ENAMODE12_ONESHOT	(1 << 6)
#define ENAMODE12_PERIODIC	(2 << 6)

/* TGCR bit definitions */
#define TIM12RS_UNRESET		(1 << 0)
#define TIM34RS_UNRESET		(1 << 1)
#define TIMMODE_64BIT_WDOG      (2 << 2)

/* WDTCR bit definitions */
#define WDEN			(1 << 14)
#define WDFLAG			(1 << 15)
#define WDKEY_SEQ0		(0xa5c6 << 16)
#define WDKEY_SEQ1		(0xda7e << 16)

/*
 * struct to hold data for each WDT device
 * @wd - hold watchdog device as is in WDT core
 * @base - base io address of WD device
 * @clk - source clock of WDT
 * @timeout - previously configured timeout
 */
struct davinci_wdt {
	struct watchdog	wd;
	void __iomem	*base;
	struct clk	*clk;
	unsigned	timeout;
};

#define to_davinci_wdt(h) container_of(h, struct davinci_wdt, wd)

static int davinci_wdt_start(struct watchdog *wd)
{
	u32 tgcr;
	u32 timer_margin;
	unsigned long wdt_freq;
	struct davinci_wdt *davinci_wdt = to_davinci_wdt(wd);

	wdt_freq = clk_get_rate(davinci_wdt->clk);

	/* disable, internal clock source */
	iowrite32(0, davinci_wdt->base + TCR);
	/* reset timer, set mode to 64-bit watchdog, and unreset */
	iowrite32(0, davinci_wdt->base + TGCR);
	tgcr = TIMMODE_64BIT_WDOG | TIM12RS_UNRESET | TIM34RS_UNRESET;
	iowrite32(tgcr, davinci_wdt->base + TGCR);
	/* clear counter regs */
	iowrite32(0, davinci_wdt->base + TIM12);
	iowrite32(0, davinci_wdt->base + TIM34);
	/* set timeout period */
	timer_margin = (((u64)davinci_wdt->timeout * wdt_freq) & 0xffffffff);
	iowrite32(timer_margin, davinci_wdt->base + PRD12);
	timer_margin = (((u64)davinci_wdt->timeout * wdt_freq) >> 32);
	iowrite32(timer_margin, davinci_wdt->base + PRD34);
	/* enable run continuously */
	iowrite32(ENAMODE12_PERIODIC, davinci_wdt->base + TCR);
	/* Once the WDT is in pre-active state write to
	 * TIM12, TIM34, PRD12, PRD34, TCR, TGCR, WDTCR are
	 * write protected (except for the WDKEY field)
	 */
	/* put watchdog in pre-active state */
	iowrite32(WDKEY_SEQ0 | WDEN, davinci_wdt->base + WDTCR);
	/* put watchdog in active state */
	iowrite32(WDKEY_SEQ1 | WDEN, davinci_wdt->base + WDTCR);
	return 0;
}

static int davinci_wdt_ping(struct watchdog *wd)
{
	struct davinci_wdt *davinci_wdt = to_davinci_wdt(wd);

	/* put watchdog in service state */
	iowrite32(WDKEY_SEQ0, davinci_wdt->base + WDTCR);
	/* put watchdog in active state */
	iowrite32(WDKEY_SEQ1, davinci_wdt->base + WDTCR);
	return 0;
}

static int davinci_wdt_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct davinci_wdt *davinci_wdt = to_davinci_wdt(wd);

	if (!davinci_wdt->timeout) {
		davinci_wdt->timeout = timeout;
		davinci_wdt_start(wd);
	} else {
		davinci_wdt_ping(wd);
		if (davinci_wdt->timeout != timeout)
			pr_warn("watchdog timeout can not be changed (currently %d)\n",
				davinci_wdt->timeout);
	}

	return 0;
}

static int davinci_wdt_probe(struct device_d *dev)
{
	struct resource *iores;
	int ret = 0;
	struct davinci_wdt *davinci_wdt;

	davinci_wdt = xzalloc(sizeof(*davinci_wdt));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	davinci_wdt->base = IOMEM(iores->start);

	davinci_wdt->clk = clk_get(dev, NULL);
	if (WARN_ON(IS_ERR(davinci_wdt->clk)))
		return PTR_ERR(davinci_wdt->clk);

	clk_enable(davinci_wdt->clk);

	davinci_wdt->wd.set_timeout = davinci_wdt_set_timeout;
	davinci_wdt->wd.hwdev = dev;

	ret = watchdog_register(&davinci_wdt->wd);
	if (ret < 0)
		dev_err(dev, "cannot register watchdog device\n");

	return ret;
}

static __maybe_unused struct of_device_id davinci_wdt_of_match[] = {
	{ .compatible = "ti,davinci-wdt", },
	{},
};

static struct driver_d platform_wdt_driver = {
	.name = "davinci-wdt",
	.of_compatible = DRV_OF_COMPAT(davinci_wdt_of_match),
	.probe = davinci_wdt_probe,
};
device_platform_driver(platform_wdt_driver);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("DaVinci Watchdog Driver");
MODULE_LICENSE("GPL");
