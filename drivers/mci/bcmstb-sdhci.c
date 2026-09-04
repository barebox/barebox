// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This code is ported from U-Boot by Andrei Lalaev <andrey.lalaev@gmail.com>
 *
 * The original code has the following copyright:
 * (C) Copyright 2018  Cisco Systems, Inc.
 * (C) Copyright 2019  Synamedia
 *
 * Author: Thomas Fitzsimmons <fitzsim@fitzsim.org>
 */

#include <common.h>
#include <init.h>
#include <mci.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "sdhci.h"

#define BCMSTB_SDHCI_MAXIMUM_CLOCK_FREQUENCY	52000000
#define BCMSTB_SDHCI_MINIMUM_CLOCK_FREQUENCY	400000

#define SDIO_CFG_CTRL				0x0
#define  SDIO_CFG_CTRL_SDCD_N_TEST_EN		BIT(31)
#define  SDIO_CFG_CTRL_SDCD_N_TEST_LEV		BIT(30)

#define SDIO_CFG_SD_PIN_SEL			0x44
#define  SDIO_CFG_SD_PIN_SEL_MASK		0x3
#define  SDIO_CFG_SD_PIN_SEL_CARD		BIT(1)

struct sdhci_bcmstb_mci_host {
	struct mci_host mci;
	void __iomem *cfg_regs;
	struct sdhci sdhci;
};

static inline struct sdhci_bcmstb_mci_host *to_sdhci_bcmstb_host(struct mci_host *mci)
{
	return container_of(mci, struct sdhci_bcmstb_mci_host, mci);
}

static int sdhci_brcmstb_init_2712(struct mci_host *mci, struct device *dev)
{
	struct sdhci_bcmstb_mci_host *host = to_sdhci_bcmstb_host(mci);
	u32 reg;
	int ret;

	ret = sdhci_reset(&host->sdhci, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL,
		     SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, 0x00);

	if (host->mci.host_caps & MMC_CAP_NONREMOVABLE) {
		/* Force presence */
		reg = readl(host->cfg_regs + SDIO_CFG_CTRL);
		reg &= ~SDIO_CFG_CTRL_SDCD_N_TEST_LEV;
		reg |= SDIO_CFG_CTRL_SDCD_N_TEST_EN;
		writel(reg, host->cfg_regs + SDIO_CFG_CTRL);
	} else {
		/* Enable card detection line */
		reg = readl(host->cfg_regs + SDIO_CFG_SD_PIN_SEL);
		reg &= ~SDIO_CFG_SD_PIN_SEL_MASK;
		reg |= SDIO_CFG_SD_PIN_SEL_CARD;
		writel(reg, host->cfg_regs + SDIO_CFG_SD_PIN_SEL);
	}

	return 0;
}

static void sdhci_brcmstb_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct sdhci_bcmstb_mci_host *host = to_sdhci_bcmstb_host(mci);

	if (ios->clock)
		sdhci_set_clock(&host->sdhci, ios->clock, host->sdhci.max_clk);
	sdhci_set_bus_width(&host->sdhci, ios->bus_width);
}

static int sdhci_brcmstb_send_cmd(struct mci_host *mci, struct mci_cmd *cmd)
{
	struct sdhci_bcmstb_mci_host *host = to_sdhci_bcmstb_host(mci);

	return sdhci_send_command(&host->sdhci, cmd);
}

static int sdhci_brcmstb_execute_tuning(struct mci_host *mci, u32 opcode)
{
	struct sdhci_bcmstb_mci_host *host = to_sdhci_bcmstb_host(mci);

	return sdhci_execute_tuning(&host->sdhci, opcode);
}

static const struct mci_ops bcmstb_sdhci_ops = {
	.init = sdhci_brcmstb_init_2712,
	.set_ios = sdhci_brcmstb_set_ios,
	.send_cmd = sdhci_brcmstb_send_cmd,
	.execute_tuning = sdhci_brcmstb_execute_tuning,
};

static int sdhci_bcmstb_probe(struct device *dev)
{
	struct sdhci_bcmstb_mci_host *host;
	struct resource *iores_cfg;
	struct resource *iores;
	struct clk *clk;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_enable(clk);
	if (ret)
		return ret;

	iores = dev_request_mem_resource_by_name(dev, "host");
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	iores_cfg = dev_request_mem_resource_by_name(dev, "cfg");
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	host = xzalloc(sizeof(*host));
	host->mci.ops = bcmstb_sdhci_ops;
	host->mci.hw_dev = dev;
	host->mci.f_min = BCMSTB_SDHCI_MINIMUM_CLOCK_FREQUENCY;
	host->mci.f_max = BCMSTB_SDHCI_MAXIMUM_CLOCK_FREQUENCY;

	host->cfg_regs = IOMEM(iores_cfg->start);

	host->sdhci.base = IOMEM(iores->start);
	host->sdhci.mci = &host->mci;

	mci_of_parse(&host->mci);

	ret = sdhci_setup_host(&host->sdhci);
	if (ret) {
		dev_err(dev, "Failed to setup SDHCI host");
		return ret;
	}

	return mci_register(&host->mci);
}

static const struct of_device_id sdhci_bcmstb_compatible[] = {
	{ .compatible = "brcm,bcm2712-sdhci" },
	{ }
};
MODULE_DEVICE_TABLE(of, sdhci_bcmstb_match);

static struct driver sdhci_bcmstb_driver = {
	.name = "sdhci-bcmstb",
	.probe = sdhci_bcmstb_probe,
	.of_compatible = DRV_OF_COMPAT(sdhci_bcmstb_compatible),
};

device_platform_driver(sdhci_bcmstb_driver);
