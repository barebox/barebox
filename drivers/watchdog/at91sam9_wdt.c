// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pengutronix, Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <watchdog.h>
#include <linux/clk.h>
#include <mach/at91_wdt.h>

#define MIN_WDT_TIMEOUT		1
#define MAX_WDT_TIMEOUT		16
#define SECS_TO_WDOG_TICKS(s)	((s) ? (((s) << 8) - 1) : 0)

struct at91sam9x_wdt {
	struct watchdog wdd;
	void __iomem *base;
};

static inline void at91sam9x_wdt_ping(struct at91sam9x_wdt *wdt)
{
	writel(AT91_WDT_WDRSTT | AT91_WDT_KEY, wdt->base + AT91_WDT_CR);
}

static int at91sam9x_wdt_set_timeout(struct watchdog *wdd, unsigned timeout)
{
	struct at91sam9x_wdt *wdt = container_of(wdd, struct at91sam9x_wdt, wdd);
	u32 mr_old, mr_new;

	mr_old = readl(wdt->base + AT91_WDT_MR);

	if (!timeout) {
		mr_new = mr_old | AT91_WDT_WDDIS;
		writel(mr_new, wdt->base + AT91_WDT_MR);
		return 0;
	}

	mr_new = AT91_WDT_WDRSTEN
		| AT91_WDT_WDDBGHLT | AT91_WDT_WDIDLEHLT
		| AT91_WDT_WDD
		| (SECS_TO_WDOG_TICKS(timeout) & AT91_WDT_WDV);

	if (mr_new != mr_old)
		writel(mr_new, wdt->base + AT91_WDT_MR);

	at91sam9x_wdt_ping(wdt);
	return 0;
}

static inline bool at91sam9x_wdt_is_disabled(struct at91sam9x_wdt *wdt)
{
	return readl(wdt->base + AT91_WDT_MR) & AT91_WDT_WDDIS;
}

static int at91sam9x_wdt_probe(struct device_d *dev)
{
	struct at91sam9x_wdt *wdt;
	struct resource *iores;
	struct clk *clk;
	int ret;

	wdt = xzalloc(sizeof(*wdt));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get watchdog memory region\n");
		return PTR_ERR(iores);
	}
	wdt->base = IOMEM(iores->start);
	clk = clk_get(dev, NULL);
	if (WARN_ON(IS_ERR(clk)))
		return PTR_ERR(clk);

	clk_enable(clk);

	wdt->wdd.set_timeout = at91sam9x_wdt_set_timeout;
	wdt->wdd.timeout_max = MAX_WDT_TIMEOUT;
	wdt->wdd.hwdev = dev;

	if (at91sam9x_wdt_is_disabled(wdt))
		wdt->wdd.running = WDOG_HW_NOT_RUNNING;
	else
		wdt->wdd.running = WDOG_HW_RUNNING;

	ret = watchdog_register(&wdt->wdd);
	if (ret)
		free(wdt);

	return ret;
}

static const __maybe_unused struct of_device_id at91sam9x_wdt_dt_ids[] = {
	{ .compatible = "atmel,at91sam9260-wdt", },
	{ .compatible = "atmel,sama5d4-wdt", },
	{ /* sentinel */ },
};

static struct driver_d at91sam9x_wdt_driver = {
	.name		= "at91sam9x-wdt",
	.of_compatible	= DRV_OF_COMPAT(at91sam9x_wdt_dt_ids),
	.probe		= at91sam9x_wdt_probe,
};

device_platform_driver(at91sam9x_wdt_driver);
