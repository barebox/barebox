// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <linux/clk.h>

struct imx_wd;

struct imx_wd_ops {
	int (*set_timeout)(struct imx_wd *, unsigned);
	void (*soc_reset)(struct imx_wd *);
	int (*init)(struct imx_wd *);
	bool (*is_running)(struct imx_wd *);
	unsigned int timeout_max;
};

struct imx_wd {
	struct watchdog wd;
	void __iomem *base;
	struct device *dev;
	const struct imx_wd_ops *ops;
	struct restart_handler restart;
	struct restart_handler restart_warm;
	bool ext_reset;
	bool bigendian;
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
#define IMX21_WDOG_WCR_WDZST	(1 << 0)
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

static void imxwd_write(struct imx_wd *priv, int reg, uint16_t val)
{
	if (priv->bigendian)
		out_be16(priv->base + reg, val);
	else
		writew(val, priv->base + reg);
}

static uint16_t imxwd_read(struct imx_wd *priv, int reg)
{
	if (priv->bigendian)
		return in_be16(priv->base + reg);
	else
		return readw(priv->base + reg);
}

static int imx1_watchdog_set_timeout(struct imx_wd *priv, unsigned timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (!timeout) {
		imxwd_write(priv, IMX1_WDOG_WCR, IMX1_WDOG_WCR_WHALT);
		return 0;
	}

	val = (timeout * 2 - 1) << 8;

	imxwd_write(priv, IMX1_WDOG_WCR, val);
	imxwd_write(priv, IMX1_WDOG_WCR, IMX1_WDOG_WCR_WDE | val);

	/* Write Service Sequence */
	imxwd_write(priv, IMX1_WDOG_WSR, 0x5555);
	imxwd_write(priv, IMX1_WDOG_WSR, 0xaaaa);

	return 0;
}

static void imx1_soc_reset(struct imx_wd *priv)
{
	writew(IMX1_WDOG_WCR_WDE, priv->base + IMX1_WDOG_WCR);
}

static inline bool imx21_watchdog_is_running(struct imx_wd *priv)
{
	return imxwd_read(priv, IMX21_WDOG_WCR) & IMX21_WDOG_WCR_WDE;
}

static int imx21_watchdog_set_timeout(struct imx_wd *priv, unsigned timeout)
{
	u16 val;

	dev_dbg(priv->dev, "%s: %d\n", __func__, timeout);

	if (timeout == 0) /* bit 2 (WDE) cannot be set to 0 again */
		return -ENOSYS;

	val = ((timeout * 2 - 1) << 8) | IMX21_WDOG_WCR_SRS |
		IMX21_WDOG_WCR_WDA;

	if (priv->ext_reset)
		val |= IMX21_WDOG_WCR_WDT;

	/* Suspend timer in low power mode */
	val |= IMX21_WDOG_WCR_WDZST;

	/*
	 * set time and some write once bits first prior enabling the
	 * watchdog according to the datasheet
	 */
	imxwd_write(priv, IMX21_WDOG_WCR, val);

	imxwd_write(priv, IMX21_WDOG_WCR, IMX21_WDOG_WCR_WDE | val);

	/* Write Service Sequence */
	imxwd_write(priv, IMX21_WDOG_WSR, 0x5555);
	imxwd_write(priv, IMX21_WDOG_WSR, 0xaaaa);

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

	imxwd_write(priv, IMX21_WDOG_WCR, val);

	/* Two additional writes due to errata ERR004346 */
	imxwd_write(priv, IMX21_WDOG_WCR, val);
	imxwd_write(priv, IMX21_WDOG_WCR, val);
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

static void __noreturn imxwd_force_soc_reset_internal(struct restart_handler *rst)
{
	struct imx_wd *priv = container_of(rst, struct imx_wd, restart_warm);

	priv->ext_reset = false;
	imxwd_force_soc_reset(&priv->restart);
}

static void imx_watchdog_detect_reset_source(struct imx_wd *priv)
{
	u16 val = imxwd_read(priv, IMX21_WDOG_WSTR);
	int priority = RESET_SOURCE_DEFAULT_PRIORITY;

	if (reset_source_get() == RESET_WDG)
		priority++;

	if (val & WSTR_COLDSTART) {
		reset_source_set_priority(RESET_POR, priority);
		return;
	}

	if (val & (WSTR_HARDRESET | WSTR_WARMSTART)) {
		reset_source_set_priority(RESET_RST, priority);
		return;
	}

	if (val & WSTR_WDOG) {
		reset_source_set_priority(RESET_WDG, priority);
		return;
	}

	/* else keep the default 'unknown' state */
}

static int imx21_wd_init_no_warm_reset(struct imx_wd *priv)
{
	imx_watchdog_detect_reset_source(priv);

	/*
	 * Disable watchdog powerdown counter
	 */
	imxwd_write(priv, IMX21_WDOG_WMCR, 0x0);

	return 0;
}

static int imx21_wd_init(struct imx_wd *priv)
{
	priv->restart_warm.name = "imxwd-warm";
	priv->restart_warm.restart = imxwd_force_soc_reset_internal;
	priv->restart_warm.priority = RESTART_DEFAULT_PRIORITY - 10;
	priv->restart_warm.flags = RESTART_FLAG_WARM_BOOTROM;

	restart_handler_register(&priv->restart_warm);

	return imx21_wd_init_no_warm_reset(priv);
}

static int imx_wd_probe(struct device *dev)
{
	struct resource *iores;
	struct imx_wd *priv;
	struct clk *clk;
	void *ops;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&ops);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(struct imx_wd));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return dev_err_probe(dev, PTR_ERR(iores),
				     "could not get memory region\n");

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "Failed to get clk\n");

	ret = clk_enable(clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable clk\n");

	priv->base = IOMEM(iores->start);
	priv->ops = ops;
	priv->wd.set_timeout = imx_watchdog_set_timeout;
	priv->wd.timeout_max = priv->ops->timeout_max;
	priv->wd.hwdev = dev;
	priv->dev = dev;
	priv->bigendian = of_device_is_big_endian(dev->of_node);

	priv->ext_reset = of_property_read_bool(dev->of_node,
						"fsl,ext-reset-output");

	if (IS_ENABLED(CONFIG_WATCHDOG_IMX)) {
		if (priv->ops->is_running) {
			if (priv->ops->is_running(priv))
				priv->wd.running = WDOG_HW_RUNNING;
			else
				priv->wd.running = WDOG_HW_NOT_RUNNING;
		}

		ret = watchdog_register(&priv->wd);
		if (ret) {
			dev_err_probe(dev, ret, "Failed to register watchdog device\n");
			goto on_error;
		}
	}

	if (priv->ops->init) {
		ret = priv->ops->init(priv);
		if (ret) {
			dev_err_probe(dev, ret,
				      "Failed to init watchdog device\n");
			goto error_unregister;
		}
	}

	dev->priv = priv;

	priv->restart.name = "imxwd";
	priv->restart.restart = imxwd_force_soc_reset;
	priv->restart.priority = RESTART_DEFAULT_PRIORITY;

	restart_handler_register(&priv->restart);

	return 0;

error_unregister:
	if (IS_ENABLED(CONFIG_WATCHDOG_IMX))
		watchdog_deregister(&priv->wd);
on_error:
	free(priv);
	return ret;
}

static const struct imx_wd_ops imx7d_wd_ops = {
	.set_timeout = imx21_watchdog_set_timeout,
	.soc_reset = imx21_soc_reset,
	.init = imx21_wd_init_no_warm_reset,
	.is_running = imx21_watchdog_is_running,
	.timeout_max = 128,
};

static const struct imx_wd_ops imx21_wd_ops = {
	.set_timeout = imx21_watchdog_set_timeout,
	.soc_reset = imx21_soc_reset,
	.init = imx21_wd_init,
	.is_running = imx21_watchdog_is_running,
	.timeout_max = 128,
};

static const struct imx_wd_ops imx1_wd_ops = {
	.set_timeout = imx1_watchdog_set_timeout,
	.soc_reset = imx1_soc_reset,
	.timeout_max = 64,
};

static __maybe_unused struct of_device_id imx_wdt_dt_ids[] = {
	{
		.compatible = "fsl,imx1-wdt",
		.data = &imx1_wd_ops,
	}, {
		.compatible = "fsl,imx21-wdt",
		.data = &imx21_wd_ops,
	}, {
		.compatible = "fsl,imx7d-wdt",
		.data = &imx7d_wd_ops,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, imx_wdt_dt_ids);

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

static struct driver imx_wd_driver = {
	.name   = "imx-watchdog",
	.probe  = imx_wd_probe,
	.of_compatible = DRV_OF_COMPAT(imx_wdt_dt_ids),
	.id_table = imx_wdt_ids,
};
device_platform_driver(imx_wd_driver);
