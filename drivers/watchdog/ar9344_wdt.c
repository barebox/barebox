/*
 *  AR9344 Watchdog driver
 *
 *  Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <of.h>
#include <reset_source.h>
#include <watchdog.h>

#define AR9344_WD_REG_CTRL		0x00
#define AR9344_WD_CTRL_LAST_RESET	BIT(31)
#define AR9344_WD_CTRL_ACTION_MASK	3
#define AR9344_WD_CTRL_ACTION_NONE	0	/* no action */
#define AR9344_WD_CTRL_ACTION_GPI	1	/* general purpose interrupt */
#define AR9344_WD_CTRL_ACTION_NMI	2	/* NMI */
#define AR9344_WD_CTRL_ACTION_FCR	3	/* full chip reset */

#define AR9344_WD_REG_TIMER		0x04

#define to_ar9344_wd(h) container_of(h, struct ar9344_wd, wd)

struct ar9344_wd {
	struct watchdog		wd;
	void __iomem		*base;
	struct clk		*clk;
	struct device_d		*dev;
};

static int ar9344_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct ar9344_wd *priv = to_ar9344_wd(wd);
	u32 val, ctrl, rate, max_timout;

	rate = clk_get_rate(priv->clk);
	max_timout = U32_MAX / rate;

	if (timeout > max_timout) {
		dev_err(priv->dev, "timeout value out of range: %d > %d\n",
			timeout, max_timout);
		return -EINVAL;
	}

	if (timeout) {
		ctrl = AR9344_WD_CTRL_ACTION_FCR;
		val = timeout * rate;
	} else {
		ctrl = AR9344_WD_CTRL_ACTION_NONE;
		val = U32_MAX;
	}

	dev_dbg(priv->dev, "%s: %d, timer:%x, ctrl: %x \n", __func__,
		timeout, val, ctrl);

	iowrite32be(val, priv->base + AR9344_WD_REG_TIMER);
	iowrite32be(ctrl, priv->base + AR9344_WD_REG_CTRL);

	return 0;
}

static void ar9344_watchdog_detect_reset_source(struct ar9344_wd *priv)
{
	u32 val = readw(priv->base + AR9344_WD_REG_CTRL);

	if (val & AR9344_WD_CTRL_LAST_RESET)
		reset_source_set(RESET_WDG);

	/* else keep the default 'unknown' state */
}

static int ar9344_wdt_probe(struct device_d *dev)
{
	struct resource *iores;
	struct ar9344_wd *priv;
	int ret;

	priv = xzalloc(sizeof(struct ar9344_wd));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		ret = PTR_ERR(iores);
		goto on_error;
	}

	priv->base = IOMEM(iores->start);
	priv->wd.set_timeout = ar9344_watchdog_set_timeout;
	priv->wd.dev = dev;
	priv->dev = dev;

	dev->priv = priv;

	ar9344_watchdog_detect_reset_source(priv);

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "could not get clk\n");
		ret = PTR_ERR(priv->clk);
		goto on_error;
	}

	clk_enable(priv->clk);

	ret = watchdog_register(&priv->wd);
	if (ret)
		goto on_error;

	return 0;

on_error:
	free(priv);
	return ret;
}

static __maybe_unused struct of_device_id ar9344_wdt_dt_ids[] = {
	{
		.compatible = "qca,ar9344-wdt",
	}, {
		/* sentinel */
	}
};

static struct driver_d ar9344_wdt_driver = {
	.name   = "ar9344-wdt",
	.probe  = ar9344_wdt_probe,
	.of_compatible = DRV_OF_COMPAT(ar9344_wdt_dt_ids),
};
device_platform_driver(ar9344_wdt_driver);
