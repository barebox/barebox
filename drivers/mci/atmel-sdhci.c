// SPDX-License-Identifier: GPL-2.0-only
/*
 * Atmel SDMMC controller driver.
 *
 * Copyright (C) 2015 Atmel,
 *		 2015 Ludovic Desroches <ludovic.desroches@atmel.com>
 *		 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <of.h>
#include <mci.h>

#include "atmel-sdhci.h"

#define ATMEL_SDHC_MIN_FREQ	400000
#define ATMEL_SDHC_GCK_RATE	240000000

struct at91_sdhci_priv {
	struct at91_sdhci host;
	struct mci_host mci;
	struct clk *hclock, *gck, *mainck;
	bool cal_always_on;
	int gck_rate;
};

#define to_priv(h) container_of(h, struct at91_sdhci_priv, mci)

static int at91_sdhci_mci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				   struct mci_data *data)
{
	return at91_sdhci_send_command(&to_priv(mci)->host, cmd, data);
}

static void at91_sdhci_mci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	at91_sdhci_set_ios(&to_priv(mci)->host, ios);
}

static int at91_sdhci_mci_init(struct mci_host *mci, struct device *dev)
{
	struct at91_sdhci_priv *priv = to_priv(mci);
	struct sdhci *sdhci = &priv->host.sdhci;
	int ret;

	priv->host.dev = dev;

	ret = sdhci_reset(sdhci, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
	if (ret)
		return ret;

	return at91_sdhci_init(&priv->host, priv->gck_rate,
			       priv->mci.non_removable, priv->cal_always_on);
}

static int at91_sdhci_conf_clks(struct at91_sdhci_priv *priv)
{
	unsigned long real_gck_rate;
	int ret;

	/*
	 * The mult clock is provided by as a generated clock by the PMC
	 * controller. In order to set the rate of gck, we have to get the
	 * base clock rate and the clock mult from capabilities.
	 */
	clk_enable(priv->hclock);
	ret = clk_set_rate(priv->gck, ATMEL_SDHC_GCK_RATE);
	if (ret < 0) {
		clk_disable(priv->hclock);
		return ret;
	}

	real_gck_rate = clk_get_rate(priv->gck);

	clk_enable(priv->mainck);
	clk_enable(priv->gck);

	return clamp_t(int, real_gck_rate, ATMEL_SDHC_MIN_FREQ, INT_MAX);
}

static void at91_sdhci_set_mci_caps(struct at91_sdhci_priv *priv)
{
	struct mci_host *mci = &priv->mci;
	at91_sdhci_host_capability(&priv->host, &mci->voltages);

	if (mci->f_max >= 26000000)
		mci->host_caps |= MMC_CAP_MMC_HIGHSPEED;
	if (mci->f_max >= 52000000)
		mci->host_caps |= MMC_CAP_MMC_HIGHSPEED_52MHZ;

	mci_of_parse(mci);
}

static int at91_sdhci_card_present(struct mci_host *mci)
{
	return at91_sdhci_is_card_inserted(&to_priv(mci)->host);
}

static int at91_sdhci_probe(struct device *dev)
{
	struct at91_sdhci_priv *priv;
	struct resource *iores;

	priv = xzalloc(sizeof(*priv));
	dev->priv = priv;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get iomem region\n");
		return PTR_ERR(iores);
	}

	priv->mainck = clk_get(dev, "baseclk");
	if (IS_ERR(priv->mainck)) {
		dev_err(dev, "failed to get baseclk\n");
		return PTR_ERR(priv->mainck);
	}

	priv->hclock = clk_get(dev, "hclock");
	if (IS_ERR(priv->hclock)) {
		dev_err(dev, "failed to get hclock\n");
		return PTR_ERR(priv->hclock);
	}

	priv->gck = clk_get(dev, "multclk");
	if (IS_ERR(priv->gck)) {
		dev_err(dev, "failed to get multclk\n");
		return PTR_ERR(priv->gck);
	}

	/*
	 * if SDCAL pin is wrongly connected, we must enable
	 * the analog calibration cell permanently.
	 */
	priv->cal_always_on = of_property_read_bool(dev->of_node,
						    "microchip,sdcal-inverted");

	at91_sdhci_mmio_init(&priv->host, IOMEM(iores->start));

	priv->gck_rate = at91_sdhci_conf_clks(priv);
	if (priv->gck_rate < 0)
		return priv->gck_rate;

	priv->mci.hw_dev = dev;
	priv->mci.send_cmd = at91_sdhci_mci_send_cmd;
	priv->mci.set_ios = at91_sdhci_mci_set_ios;
	priv->mci.init = at91_sdhci_mci_init;
	priv->mci.f_max = priv->gck_rate;
	priv->mci.f_min = ATMEL_SDHC_MIN_FREQ;
	priv->mci.card_present = at91_sdhci_card_present;

	at91_sdhci_set_mci_caps(priv);

	return mci_register(&priv->mci);
}

static const struct of_device_id at91_sdhci_dt_match[] = {
	{ .compatible = "atmel,sama5d2-sdhci"  },
	{ .compatible = "microchip,sam9x60-sdhci" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, at91_sdhci_dt_match);

static struct driver at91_sdhci_driver = {
	.name		= "sdhci-at91",
	.of_compatible	= DRV_OF_COMPAT(at91_sdhci_dt_match),
	.probe		= at91_sdhci_probe,
};
device_platform_driver(at91_sdhci_driver);
