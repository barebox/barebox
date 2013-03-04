/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Note: this driver works for the i.MX28 SoC. It might work for the
 * i.MX23 Soc as well, but is not tested yet.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>
#include <reset_source.h>

#define MXS_RTC_CTRL 0x0
#define MXS_RTC_SET_ADDR 0x4
#define MXS_RTC_CLR_ADDR 0x8
# define MXS_RTC_CTRL_WATCHDOGEN (1 << 4)

#define MXS_RTC_STAT 0x10
# define MXS_RTC_STAT_WD_PRESENT (1 << 29)

#define MXS_RTC_WATCHDOG 0x50

#define MXS_RTC_PERSISTENT0 0x60
/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT0_EXT_RST (1 << 21)
/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT0_THM_RST (1 << 20)

#define MXS_RTC_PERSISTENT1 0x70
/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT1_FORCE_UPDATER (1 << 31)

#define MXS_RTC_DEBUG 0xc0

#define WDOG_TICK_RATE 1000 /* the watchdog uses a 1 kHz clock rate */

struct imx28_wd {
	struct watchdog wd;
	void __iomem *regs;
};

#define to_imx28_wd(h) container_of(h, struct imx28_wd, wd)

static int imx28_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct imx28_wd *pwd = (struct imx28_wd *)to_imx28_wd(wd);
	void __iomem *base;

	if (timeout > (ULONG_MAX / WDOG_TICK_RATE))
		return -EINVAL;

	if (timeout) {
		writel(timeout * WDOG_TICK_RATE, pwd->regs + MXS_RTC_WATCHDOG);
		base = pwd->regs + MXS_RTC_SET_ADDR;
	} else {
		base = pwd->regs + MXS_RTC_CLR_ADDR;
	}
	writel(MXS_RTC_CTRL_WATCHDOGEN, base + MXS_RTC_CTRL);
	writel(MXS_RTC_PERSISTENT1_FORCE_UPDATER, base + MXS_RTC_PERSISTENT1);

	return 0;
}

static void __maybe_unused imx28_detect_reset_source(const struct imx28_wd *p)
{
	u32 reg;

	reg = readl(p->regs + MXS_RTC_PERSISTENT0);
	if (reg & MXS_RTC_PERSISTENT0_EXT_RST) {
		writel(MXS_RTC_PERSISTENT0_EXT_RST,
			p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
		/*
		 * if the RTC has woken up the SoC, additionally the ALARM_WAKE
		 * bit is set. This bit should have precedence, because it
		 * reports the real event, why we are here.
		 */
		if (reg & MXS_RTC_PERSISTENT0_ALARM_WAKE) {
			writel(MXS_RTC_PERSISTENT0_ALARM_WAKE,
				p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
			set_reset_source(RESET_WKE);
			return;
		}
		set_reset_source(RESET_POR);
		return;
	}
	if (reg & MXS_RTC_PERSISTENT0_THM_RST) {
		writel(MXS_RTC_PERSISTENT0_THM_RST,
			p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
		set_reset_source(RESET_RST);
		return;
	}

	set_reset_source(RESET_RST);
}

static int imx28_wd_probe(struct device_d *dev)
{
	struct imx28_wd *priv;
	int rc;

	priv = xzalloc(sizeof(struct imx28_wd));
	priv->regs = dev_request_mem_region(dev, 0);
	priv->wd.set_timeout = imx28_watchdog_set_timeout;

	if (!(readl(priv->regs + MXS_RTC_STAT) & MXS_RTC_STAT_WD_PRESENT)) {
		rc = -ENODEV;
		goto on_error;
	}

	/* disable the debug feature to ensure a working WD */
	writel(0x00000000, priv->regs + MXS_RTC_DEBUG);

	rc = watchdog_register(&priv->wd);
	if (rc != 0)
		goto on_error;

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		imx28_detect_reset_source(priv);

	dev->priv = priv;
	return 0;

on_error:
	free(priv);
	return rc;
}

static void imx28_wd_remove(struct device_d *dev)
{
	struct imx28_wd *priv= dev->priv;
	watchdog_deregister(&priv->wd);
	free(priv);
}

static struct driver_d imx28_wd_driver = {
	.name   = "im28wd",
	.probe  = imx28_wd_probe,
	.remove = imx28_wd_remove,
};
device_platform_driver(imx28_wd_driver);
