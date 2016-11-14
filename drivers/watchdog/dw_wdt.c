/*
 * Copyright 2010-2011 Picochip Ltd., Jamie Iles
 * http://www.picochip.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This file implements a driver for the Synopsys DesignWare watchdog device
 * in the many subsystems. The watchdog has 16 different timeout periods
 * and these are a function of the input clock frequency.
 *
 * The DesignWare watchdog cannot be stopped once it has been started so we
 * do not implement a stop function. The watchdog core will continue to send
 * heartbeat requests after the watchdog device has been closed.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <restart.h>
#include <watchdog.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/reset.h>

#define WDOG_CONTROL_REG_OFFSET		    0x00
#define WDOG_CONTROL_REG_WDT_EN_MASK	    0x01
#define WDOG_TIMEOUT_RANGE_REG_OFFSET	    0x04
#define WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT    4
#define WDOG_CURRENT_COUNT_REG_OFFSET	    0x08
#define WDOG_COUNTER_RESTART_REG_OFFSET     0x0c
#define WDOG_COUNTER_RESTART_KICK_VALUE	    0x76

/* The maximum TOP (timeout period) value that can be set in the watchdog. */
#define DW_WDT_MAX_TOP		15

#define DW_WDT_DEFAULT_SECONDS	30

struct dw_wdt {
	void __iomem		*regs;
	struct clk		*clk;
	struct restart_handler	restart;
	struct watchdog		wdd;
	struct reset_control	*rst;
};

#define to_dw_wdt(wdd)	container_of(wdd, struct dw_wdt, wdd)

static inline int dw_wdt_top_in_seconds(struct dw_wdt *dw_wdt, unsigned top)
{
	/*
	 * There are 16 possible timeout values in 0..15 where the number of
	 * cycles is 2 ^ (16 + i) and the watchdog counts down.
	 */
	return (1U << (16 + top)) / clk_get_rate(dw_wdt->clk);
}

static int dw_wdt_start(struct watchdog *wdd)
{
	struct dw_wdt *dw_wdt = to_dw_wdt(wdd);

	writel(WDOG_CONTROL_REG_WDT_EN_MASK,
	       dw_wdt->regs + WDOG_CONTROL_REG_OFFSET);

	return 0;
}

static int dw_wdt_stop(struct watchdog *wdd)
{
	struct dw_wdt *dw_wdt = to_dw_wdt(wdd);

	if (IS_ERR(dw_wdt->rst)) {
		dev_warn(dw_wdt->wdd.dev, "No reset line. Will not stop.\n");
		return PTR_ERR(dw_wdt->rst);
	}

	reset_control_assert(dw_wdt->rst);
	reset_control_deassert(dw_wdt->rst);

	return 0;
}

static int dw_wdt_set_timeout(struct watchdog *wdd, unsigned int top_s)
{
	struct dw_wdt *dw_wdt = to_dw_wdt(wdd);
	int i, top_val = DW_WDT_MAX_TOP;

	if (top_s == 0)
		return dw_wdt_stop(wdd);

	/*
	 * Iterate over the timeout values until we find the closest match. We
	 * always look for >=.
	 */
	for (i = 0; i <= DW_WDT_MAX_TOP; ++i) {
		if (dw_wdt_top_in_seconds(dw_wdt, i) >= top_s) {
			top_val = i;
			break;
		}
	}

	/*
	 * Set the new value in the watchdog.  Some versions of dw_wdt
	 * have have TOPINIT in the TIMEOUT_RANGE register (as per
	 * CP_WDT_DUAL_TOP in WDT_COMP_PARAMS_1).  On those we
	 * effectively get a pat of the watchdog right here.
	 */
	writel(top_val | top_val << WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT,
	       dw_wdt->regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);

	dw_wdt_start(wdd);

	return 0;
}

static void __noreturn dw_wdt_restart_handle(struct restart_handler *rst)
{
	struct dw_wdt *dw_wdt;

	dw_wdt = container_of(rst, struct dw_wdt, restart);

	dw_wdt->wdd.set_timeout(&dw_wdt->wdd, -1);

	mdelay(1000);

	hang();
}

static int dw_wdt_drv_probe(struct device_d *dev)
{
	struct watchdog *wdd;
	struct dw_wdt *dw_wdt;
	struct resource *mem;
	int ret;

	dw_wdt = xzalloc(sizeof(*dw_wdt));

	mem = dev_request_mem_resource(dev, 0);
	dw_wdt->regs = IOMEM(mem->start);
	if (IS_ERR(dw_wdt->regs))
		return PTR_ERR(dw_wdt->regs);

	dw_wdt->clk = clk_get(dev, NULL);
	if (IS_ERR(dw_wdt->clk))
		return PTR_ERR(dw_wdt->clk);

	ret = clk_enable(dw_wdt->clk);
	if (ret)
		return ret;

	dw_wdt->rst = reset_control_get(dev, NULL);
	if (IS_ERR(dw_wdt->rst))
		dev_warn(dev, "No reset lines. Will not be able to stop once started.\n");

	wdd = &dw_wdt->wdd;
	wdd->name = "dw_wdt";
	wdd->dev = dev;
	wdd->set_timeout = dw_wdt_set_timeout;

	ret = watchdog_register(wdd);
	if (ret)
		goto out_disable_clk;

	dw_wdt->restart.name = "dw_wdt";
	dw_wdt->restart.restart = dw_wdt_restart_handle;

	ret = restart_handler_register(&dw_wdt->restart);
	if (ret)
		dev_warn(dev, "cannot register restart handler\n");

	if (!IS_ERR(dw_wdt->rst))
		reset_control_deassert(dw_wdt->rst);

	return 0;

out_disable_clk:
	clk_disable(dw_wdt->clk);
	return ret;
}

static struct of_device_id dw_wdt_of_match[] = {
	{ .compatible = "snps,dw-wdt", },
	{ /* sentinel */ }
};

static struct driver_d dw_wdt_driver = {
	.probe		= dw_wdt_drv_probe,
	.of_compatible	= DRV_OF_COMPAT(dw_wdt_of_match),
};
device_platform_driver(dw_wdt_driver);
