/*
 * omap_wdt.c
 *
 * Watchdog driver for the TI OMAP 16xx & 24xx/34xx 32KHz (non-secure) watchdog
 *
 * Copyright (c) 2015 Phytec Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 *
 * Based on Linux Kernel OMAP WTD driver:
 *
 * Author: MontaVista Software, Inc.
 *	 <gdavis@mvista.com> or <source@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * History:
 *
 * 20030527: George G. Davis <gdavis@mvista.com>
 *	Initially based on linux-2.4.19-rmk7-pxa1/drivers/char/sa1100_wdt.c
 *	(c) Copyright 2000 Oleg Drokin <green@crimea.edu>
 *	Based on SoftDog driver by Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 * Copyright (c) 2004 Texas Instruments.
 *	1. Modified to support OMAP1610 32-KHz watchdog timer
 *	2. Ported to 2.6 kernel
 *
 * Copyright (c) 2005 David Brownell
 *	Use the driver model and standard identifiers; handle bigger timeouts.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>

#define OMAP_WATCHDOG_REV		(0x00)
#define OMAP_WATCHDOG_SYS_CONFIG	(0x10)
#define OMAP_WATCHDOG_STATUS		(0x14)
#define OMAP_WATCHDOG_CNTRL		(0x24)
#define OMAP_WATCHDOG_CRR		(0x28)
#define OMAP_WATCHDOG_LDR		(0x2c)
#define OMAP_WATCHDOG_TGR		(0x30)
#define OMAP_WATCHDOG_WPS		(0x34)
#define OMAP_WATCHDOG_SPR		(0x48)

#define TIMER_MARGIN_DEFAULT	60	/* 60 secs */

#define PTV			0	/* prescale */
#define GET_WLDR_VAL(secs)      (0xffffffff - ((secs) * (32768/(1<<PTV))) + 1)
#define GET_WCCR_SECS(val)      ((0xffffffff - (val) + 1) / (32768/(1<<PTV)))

#define to_omap_wdt_dev(_wdog)	container_of(_wdog, struct omap_wdt_dev, wdog)

struct omap_wdt_dev {
	struct watchdog wdog;
	void __iomem    *base;
	int		wdt_trgr_pattern;
	unsigned	timeout;
};

static void omap_wdt_reload(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/* wait for posted write to complete */
	while ((readl(base + OMAP_WATCHDOG_WPS)) & 0x08);

	wdev->wdt_trgr_pattern = ~wdev->wdt_trgr_pattern;
	writel(wdev->wdt_trgr_pattern, (base + OMAP_WATCHDOG_TGR));

	/* wait for posted write to complete */
	while ((readl(base + OMAP_WATCHDOG_WPS)) & 0x08);
	/* reloaded WCRR from WLDR */
}

static void omap_wdt_enable(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/* Sequence to enable the watchdog */
	writel(0xBBBB, base + OMAP_WATCHDOG_SPR);
	while ((readl(base + OMAP_WATCHDOG_WPS)) & 0x10);

	writel(0x4444, base + OMAP_WATCHDOG_SPR);
	while ((readl(base + OMAP_WATCHDOG_WPS)) & 0x10);
}

static void omap_wdt_disable(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/* sequence required to disable watchdog */
	writel(0xAAAA, base + OMAP_WATCHDOG_SPR);	/* TIMER_MODE */
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x10);

	writel(0x5555, base + OMAP_WATCHDOG_SPR);	/* TIMER_MODE */
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x10);
}

static void omap_wdt_set_timer(struct omap_wdt_dev *wdev,
				   unsigned int timeout)
{
	u32 pre_margin = GET_WLDR_VAL(timeout);
	void __iomem *base = wdev->base;

	/* just count up at 32 KHz */
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x04);

	writel(pre_margin, base + OMAP_WATCHDOG_LDR);
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x04);

}

static void omap_wdt_init(struct omap_wdt_dev *wdev)
{
	void __iomem *base = wdev->base;

	/*
	 * Make sure the watchdog is disabled. This is unfortunately required
	 * because writing to various registers with the watchdog running has no
	 * effect.
	 */
	omap_wdt_disable(wdev);

	/* initialize prescaler */
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x01);

	writel((1 << 5) | (PTV << 2), base + OMAP_WATCHDOG_CNTRL);
	while (readl(base + OMAP_WATCHDOG_WPS) & 0x01);

	omap_wdt_set_timer(wdev, wdev->timeout);
	omap_wdt_reload(wdev); /* trigger loading of new timeout value */
}

static int omap_wdt_set_timeout(struct watchdog *wdog,
				unsigned int timeout)
{
	struct omap_wdt_dev *wdev = to_omap_wdt_dev(wdog);

	omap_wdt_disable(wdev);
	omap_wdt_set_timer(wdev, timeout);
	omap_wdt_enable(wdev);
	omap_wdt_reload(wdev);
	wdev->timeout = timeout;

	return 0;
}

static int omap_wdt_probe(struct device_d *dev)
{
	struct resource *iores;
	struct omap_wdt_dev *wdev;
	int ret;

	wdev = xzalloc(sizeof(*wdev));
	wdev->wdog.set_timeout	= omap_wdt_set_timeout;
	wdev->wdog.hwdev = dev;
	wdev->wdt_trgr_pattern	= 0x1234;

	/* reserve static register mappings */
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto error;
	}
	wdev->base = IOMEM(iores->start);

	wdev->timeout = TIMER_MARGIN_DEFAULT;

	ret = watchdog_register(&wdev->wdog);
	if (ret)
		goto error;

	dev_info(dev, "OMAP Watchdog Timer Rev 0x%02x\n",
		readl(wdev->base + OMAP_WATCHDOG_REV) & 0xFF);

	omap_wdt_init(wdev);

	return 0;

error:
	free(wdev);
	return ret;
}

static const struct of_device_id omap_wdt_of_match[] = {
	{ .compatible = "ti,omap3-wdt", },
	{},
};

static struct driver_d omap_wdt_driver = {
	.probe		= omap_wdt_probe,
	.name	= "omap_wdt",
	.of_compatible = DRV_OF_COMPAT(omap_wdt_of_match),
};
device_platform_driver(omap_wdt_driver);
