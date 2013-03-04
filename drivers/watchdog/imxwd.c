/*
 * (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>
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
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>
#include <reset_source.h>

struct imx_wd {
	struct watchdog wd;
	void __iomem *base;
	struct device_d *dev;
	int (*set_timeout)(struct imx_wd *, unsigned);
};

#define to_imx_wd(h) container_of(h, struct imx_wd, wd)

#define IMX1_WDOG_WCR	0x00 /* Watchdog Control Register */
#define IMX1_WDOG_WSR	0x04 /* Watchdog Service Register */
#define IMX1_WDOG_WSTR	0x08 /* Watchdog Status Register  */
#define IMX1_WDOG_WCR_WDE	(1 << 0)
#define IMX1_WDOG_WCR_WHALT	(1 << 15)

#define IMX21_WDOG_WCR	0x00 /* Watchdog Control Register */
#define IMX21_WDOG_WSR	0x02 /* Watchdog Service Register */
#define IMX21_WDOG_WSTR	0x04 /* Watchdog Status Register  */
#define IMX21_WDOG_WCR_WDE	(1 << 2)
#define IMX21_WDOG_WCR_SRS	(1 << 4)
#define IMX21_WDOG_WCR_WDA	(1 << 5)

/* valid for i.MX25, i.MX27, i.MX31, i.MX35, i.MX51 */
#define WSTR_WARMSTART	(1 << 0)
/* valid for i.MX25, i.MX27, i.MX31, i.MX35, i.MX51 */
#define WSTR_WDOG	(1 << 1)
/* valid for i.MX27, i.MX31, always '0' on i.MX25, i.MX35, i.MX51 */
#define WSTR_HARDRESET	(1 << 3)
/* valid for i.MX27, i.MX31, always '0' on i.MX25, i.MX35, i.MX51 */
#define WSTR_COLDSTART	(1 << 4)

static int imx1_watchdog_set_timeout(struct imx_wd *priv, int timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (timeout > 64)
		return -EINVAL;

	if (!timeout) {
		writew(IMX1_WDOG_WCR_WHALT, priv->base + IMX1_WDOG_WCR);
		return 0;
	}

	if (timeout > 0)
		val = (timeout * 2 - 1) << 8;
	else
		val = 0;

	writew(val, priv->base + IMX1_WDOG_WCR);
	writew(IMX1_WDOG_WCR_WDE | val, priv->base + IMX1_WDOG_WCR);

	/* Write Service Sequence */
	writew(0x5555, priv->base + IMX1_WDOG_WSR);
	writew(0xaaaa, priv->base + IMX1_WDOG_WSR);

	return 0;
}

static int imx21_watchdog_set_timeout(struct imx_wd *priv, int timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (!timeout || timeout > 128)
		return -EINVAL;

	if (timeout > 0)
		val = ((timeout * 2 - 1) << 8) | IMX21_WDOG_WCR_SRS |
			IMX21_WDOG_WCR_WDA;
	else
		val = 0;

	writew(val, priv->base + IMX21_WDOG_WCR);
	writew(IMX21_WDOG_WCR_WDE | val, priv->base + IMX21_WDOG_WCR);

	/* Write Service Sequence */
	writew(0x5555, priv->base + IMX21_WDOG_WSR);
	writew(0xaaaa, priv->base + IMX21_WDOG_WSR);

	return 0;
}

static int imx_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct imx_wd *priv = (struct imx_wd *)to_imx_wd(wd);

	return priv->set_timeout(priv, timeout);
}

static struct imx_wd *reset_wd;

void __noreturn reset_cpu(unsigned long addr)
{
	if (reset_wd)
		reset_wd->set_timeout(reset_wd, -1);

	mdelay(1000);

	hang();
}

static void imx_watchdog_detect_reset_source(struct imx_wd *priv)
{
	u16 val = readw(priv->base + IMX21_WDOG_WSTR);

	if (val & WSTR_COLDSTART) {
		set_reset_source(RESET_POR);
		return;
	}

	if (val & (WSTR_HARDRESET | WSTR_WARMSTART)) {
		set_reset_source(RESET_RST);
		return;
	}

	if (val & WSTR_WDOG) {
		set_reset_source(RESET_WDG);
		return;
	}

	/* else keep the default 'unknown' state */
}

static int imx_wd_probe(struct device_d *dev)
{
	struct imx_wd *priv;
	void *fn;
	int ret;

	ret = dev_get_drvdata(dev, (unsigned long *)&fn);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(struct imx_wd));
	priv->base = dev_request_mem_region(dev, 0);
	priv->set_timeout = fn;
	priv->wd.set_timeout = imx_watchdog_set_timeout;
	priv->dev = dev;

	if (!reset_wd)
		reset_wd = priv;

	if (IS_ENABLED(CONFIG_WATCHDOG_IMX)) {
		ret = watchdog_register(&priv->wd);
		if (ret)
			goto on_error;
	}

	if (fn != imx1_watchdog_set_timeout)
		imx_watchdog_detect_reset_source(priv);

	dev->priv = priv;

	return 0;

on_error:
	if (reset_wd && reset_wd != priv)
		free(priv);
	return ret;
}

static void imx_wd_remove(struct device_d *dev)
{
	struct imx_wd *priv = dev->priv;

	if (IS_ENABLED(CONFIG_WATCHDOG_IMX))
		watchdog_deregister(&priv->wd);

	if (reset_wd && reset_wd != priv)
		free(priv);
}

static __maybe_unused struct of_device_id imx_wdt_dt_ids[] = {
	{
		.compatible = "fsl,imx1-wdt",
		.data = (unsigned long)&imx1_watchdog_set_timeout,
	}, {
		.compatible = "fsl,imx21-wdt",
		.data = (unsigned long)&imx21_watchdog_set_timeout,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_wdt_ids[] = {
	{
		.name = "imx1-wdt",
		.driver_data = (unsigned long)&imx1_watchdog_set_timeout,
	}, {
		.name = "imx21-wdt",
		.driver_data = (unsigned long)&imx21_watchdog_set_timeout,
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_wd_driver = {
	.name   = "imx-watchdog",
	.probe  = imx_wd_probe,
	.remove = imx_wd_remove,
	.of_compatible = DRV_OF_COMPAT(imx_wdt_dt_ids),
	.id_table = imx_wdt_ids,
};
device_platform_driver(imx_wd_driver);
