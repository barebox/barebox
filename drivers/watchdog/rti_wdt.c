// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *   Jan Kiszka <jan.kiszka@siemens.com>
 *
 * Derived from linux/drivers/watchdog/rti_wdt.c
 */
#include <init.h>
#include <io.h>
#include <of.h>
#include <clock.h>
#include <malloc.h>
#include <watchdog.h>
#include <driver.h>
#include <linux/clk.h>
#include <linux/math64.h>

/* Timer register set definition */
#define RTIDWDCTRL		0x90
#define RTIDWDPRLD		0x94
#define RTIWDSTATUS		0x98
#define RTIWDKEY		0x9c
#define RTIDWDCNTR		0xa0
#define RTIWWDRXCTRL		0xa4
#define RTIWWDSIZECTRL		0xa8

#define RTIWWDRXN_RST 		0x5
#define RTIWWDRXN_NMI		0xa

#define RTIWWDSIZE_100P		0x5
#define RTIWWDSIZE_50P		0x50

#define WDENABLE_KEY		0xa98559da

#define WDKEY_SEQ0		0xe51a
#define WDKEY_SEQ1		0xa35c

#define WDT_PRELOAD_SHIFT	13

#define WDT_PRELOAD_MAX		0xfff

#define DWDST			BIT(1)

struct rti_wdt_data {
	bool nmi;
};

struct rti_wdt_priv {
	void __iomem *regs;
	struct watchdog wdt;
	unsigned int clk_hz;
	struct rti_wdt_data *data;
};

static int rti_wdt_ping(struct watchdog *wdt)
{
	struct rti_wdt_priv *priv = container_of(wdt, struct rti_wdt_priv, wdt);
	u64 halftime;

	halftime = wdt->timeout_cur / 2 + 1;

	if (!is_timeout_non_interruptible(wdt->last_ping, halftime * SECOND))
		return 0;

	writel(WDKEY_SEQ0, priv->regs + RTIWDKEY);
	writel(WDKEY_SEQ1, priv->regs + RTIWDKEY);

	return 0;
}

static int rti_wdt_settimeout(struct watchdog *wdt, unsigned int timeout)
{
	struct rti_wdt_priv *priv = container_of(wdt, struct rti_wdt_priv, wdt);
	u32 timer_margin, reaction;

	if (!timeout)
		return -ENOSYS;

	if (wdt->running == WDOG_HW_RUNNING && timeout == wdt->timeout_cur)
		return rti_wdt_ping(wdt);

	if (readl(priv->regs + RTIDWDCTRL) == WDENABLE_KEY)
		return -ENOSYS;

	timer_margin = timeout * priv->clk_hz;
	timer_margin >>= WDT_PRELOAD_SHIFT;
	if (timer_margin > WDT_PRELOAD_MAX)
		timer_margin = WDT_PRELOAD_MAX;

	writel(timer_margin, priv->regs + RTIDWDPRLD);

	if (priv->data->nmi)
		reaction = RTIWWDRXN_NMI;
	else
		reaction = RTIWWDRXN_RST;

	writel(reaction, priv->regs + RTIWWDRXCTRL);
	writel(RTIWWDSIZE_50P, priv->regs + RTIWWDSIZECTRL);

	(void)readl(priv->regs + RTIWWDSIZECTRL);

	writel(WDENABLE_KEY, priv->regs + RTIDWDCTRL);

	return 0;
}

static unsigned int rti_wdt_get_timeleft_s(struct watchdog *wdt)
{                       
	struct rti_wdt_priv *priv = container_of(wdt, struct rti_wdt_priv, wdt);
	u32 timer_counter;
	u32 val;
                
	/* if timeout has occurred then return 0 */
	val = readl(priv->regs + RTIWDSTATUS);
	if (val & DWDST)
		return 0;

	timer_counter = readl(priv->regs + RTIDWDCNTR);
                        
	return timer_counter / priv->clk_hz;
}

static int rti_wdt_probe(struct device *dev)
{
	struct rti_wdt_priv *priv;
	struct clk *clk;
	struct watchdog *wdt;
	static bool one = false;

	if (one)
		return 0;
	one = true;

	priv = xzalloc(sizeof(*priv));

	wdt = &priv->wdt;

	priv->regs = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->regs))
		return -EINVAL;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "No clock");

	priv->clk_hz = clk_get_rate(clk);
	if (priv->clk_hz == 0)
		return -EINVAL;

	priv->data = device_get_match_data(dev);

	/*
	 * If watchdog is running at 32k clock, it is not accurate.
	 * Adjust frequency down in this case so that it does not expire
	 * earlier than expected.
	 */
	if (priv->clk_hz < 32768)
		priv->clk_hz = priv->clk_hz * 9 / 10;

	wdt = &priv->wdt;
        wdt->name = "rti_wdt";
        wdt->hwdev = dev;
        wdt->set_timeout = rti_wdt_settimeout;
	wdt->ping = rti_wdt_ping;
        wdt->timeout_max = WDT_PRELOAD_MAX / (priv->clk_hz >> WDT_PRELOAD_SHIFT);

	if (readl(priv->regs + RTIDWDCTRL) == WDENABLE_KEY) {
		u64 heartbeat_s;
		u32 last_ping_s;

		wdt->running = WDOG_HW_RUNNING;

		heartbeat_s = readl(priv->regs + RTIDWDPRLD);
		heartbeat_s <<= WDT_PRELOAD_SHIFT;
		do_div(heartbeat_s, priv->clk_hz);
		wdt->timeout_cur = heartbeat_s;
		last_ping_s = heartbeat_s - rti_wdt_get_timeleft_s(wdt) + 1;

		wdt->last_ping = get_time_ns() - last_ping_s * SECOND;
	} else {
		wdt->running = WDOG_HW_NOT_RUNNING;
	}

	return watchdog_register(wdt);
}

static struct rti_wdt_data j7_wdt = {
	.nmi = true,
};

static struct rti_wdt_data am62l_wdt = {
	.nmi = false,
};

static const struct of_device_id rti_wdt_of_match[] = {
	{ .compatible = "ti,j7-rti-wdt", .data = &j7_wdt },
	{ .compatible = "ti,am62l-rti-wdt", .data = &am62l_wdt },
	{ /* sentinel */ }
};

static struct driver rti_wdt_driver = {
	.name = "rti-wdt",
	.probe = rti_wdt_probe,
	.of_match_table = rti_wdt_of_match,
};
device_platform_driver(rti_wdt_driver);
