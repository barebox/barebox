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
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <restart.h>
#include <watchdog.h>
#include <reset_source.h>

struct imx_wd;

struct imx_wd_ops {
	int (*set_timeout)(struct imx_wd *, unsigned);
	void (*soc_reset)(struct imx_wd *);
	int (*init)(struct imx_wd *);
};

struct imx_wd {
	struct watchdog wd;
	void __iomem *base;
	struct device_d *dev;
	const struct imx_wd_ops *ops;
	struct restart_handler restart;
	bool ext_reset;
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
#define IMX21_WDOG_WMCR	0x08 /* Misc Register */
#define IMX21_WDOG_WCR_WDE	(1 << 2)
#define IMX21_WDOG_WCR_WDT	(1 << 3)
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

static int imx1_watchdog_set_timeout(struct imx_wd *priv, unsigned timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (timeout > 64)
		return -EINVAL;

	if (!timeout) {
		writew(IMX1_WDOG_WCR_WHALT, priv->base + IMX1_WDOG_WCR);
		return 0;
	}

	val = (timeout * 2 - 1) << 8;

	writew(val, priv->base + IMX1_WDOG_WCR);
	writew(IMX1_WDOG_WCR_WDE | val, priv->base + IMX1_WDOG_WCR);

	/* Write Service Sequence */
	writew(0x5555, priv->base + IMX1_WDOG_WSR);
	writew(0xaaaa, priv->base + IMX1_WDOG_WSR);

	return 0;
}

static void imx1_soc_reset(struct imx_wd *priv)
{
	writew(IMX1_WDOG_WCR_WDE, priv->base + IMX1_WDOG_WCR);
}

static int imx21_watchdog_set_timeout(struct imx_wd *priv, unsigned timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (timeout > 128)
		return -EINVAL;

	if (timeout == 0) /* bit 2 (WDE) cannot be set to 0 again */
		return -ENOSYS;

	val = ((timeout * 2 - 1) << 8) | IMX21_WDOG_WCR_SRS |
		IMX21_WDOG_WCR_WDA;

	if (priv->ext_reset)
		val |= IMX21_WDOG_WCR_WDT;

	writew(IMX21_WDOG_WCR_WDE | val, priv->base + IMX21_WDOG_WCR);

	/* Write Service Sequence */
	writew(0x5555, priv->base + IMX21_WDOG_WSR);
	writew(0xaaaa, priv->base + IMX21_WDOG_WSR);

	return 0;
}

static void imx21_soc_reset(struct imx_wd *priv)
{
	u16 val = 0;

	/* Use internal reset or external - not both */
	if (priv->ext_reset)
		val |= IMX21_WDOG_WCR_SRS; /* do not assert int reset */
	else
		val |= IMX21_WDOG_WCR_WDA; /* do not assert ext-reset */

	writew(val, priv->base + IMX21_WDOG_WCR);

	/* Two additional writes due to errata ERR004346 */
	writew(val, priv->base + IMX21_WDOG_WCR);
	writew(val, priv->base + IMX21_WDOG_WCR);
}

static int imx_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct imx_wd *priv = (struct imx_wd *)to_imx_wd(wd);

	return priv->ops->set_timeout(priv, timeout);
}

static void __noreturn imxwd_force_soc_reset(struct restart_handler *rst)
{
	struct imx_wd *priv = container_of(rst, struct imx_wd, restart);

	priv->ops->soc_reset(priv);

	mdelay(1000);

	hang();
}

static void imx_watchdog_detect_reset_source(struct imx_wd *priv)
{
	u16 val = readw(priv->base + IMX21_WDOG_WSTR);

	if (val & WSTR_COLDSTART) {
		reset_source_set(RESET_POR);
		return;
	}

	if (val & (WSTR_HARDRESET | WSTR_WARMSTART)) {
		reset_source_set(RESET_RST);
		return;
	}

	if (val & WSTR_WDOG) {
		reset_source_set(RESET_WDG);
		return;
	}

	/* else keep the default 'unknown' state */
}

static int imx21_wd_init(struct imx_wd *priv)
{
	imx_watchdog_detect_reset_source(priv);

	/*
	 * Disable watchdog powerdown counter
	 */
	writew(0x0, priv->base + IMX21_WDOG_WMCR);

	return 0;
}

static int imx_wd_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imx_wd *priv;
	void *ops;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&ops);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(struct imx_wd));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	priv->base = IOMEM(iores->start);
	priv->ops = ops;
	priv->wd.set_timeout = imx_watchdog_set_timeout;
	priv->wd.dev = dev;
	priv->dev = dev;

	priv->ext_reset = of_property_read_bool(dev->device_node,
						"fsl,ext-reset-output");

	if (IS_ENABLED(CONFIG_WATCHDOG_IMX)) {
		ret = watchdog_register(&priv->wd);
		if (ret)
			goto on_error;
	}

	if (priv->ops->init) {
		ret = priv->ops->init(priv);
		if (ret) {
			dev_err(dev, "Failed to init watchdog device %d\n", ret);
			goto error_unregister;
		}
	}

	dev->priv = priv;

	priv->restart.name = "imxwd";
	priv->restart.restart = imxwd_force_soc_reset;

	restart_handler_register(&priv->restart);

	return 0;

error_unregister:
	if (IS_ENABLED(CONFIG_WATCHDOG_IMX))
		watchdog_deregister(&priv->wd);
on_error:
	free(priv);
	return ret;
}

static const struct imx_wd_ops imx21_wd_ops = {
	.set_timeout = imx21_watchdog_set_timeout,
	.soc_reset = imx21_soc_reset,
	.init = imx21_wd_init,
};

static const struct imx_wd_ops imx1_wd_ops = {
	.set_timeout = imx1_watchdog_set_timeout,
	.soc_reset = imx1_soc_reset,
};

static __maybe_unused struct of_device_id imx_wdt_dt_ids[] = {
	{
		.compatible = "fsl,imx1-wdt",
		.data = &imx1_wd_ops,
	}, {
		.compatible = "fsl,imx21-wdt",
		.data = &imx21_wd_ops,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_wdt_ids[] = {
	{
		.name = "imx1-wdt",
		.driver_data = (unsigned long)&imx1_wd_ops,
	}, {
		.name = "imx21-wdt",
		.driver_data = (unsigned long)&imx21_wd_ops,
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_wd_driver = {
	.name   = "imx-watchdog",
	.probe  = imx_wd_probe,
	.of_compatible = DRV_OF_COMPAT(imx_wdt_dt_ids),
	.id_table = imx_wdt_ids,
};
device_platform_driver(imx_wd_driver);
