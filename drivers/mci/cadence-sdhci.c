// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <mci.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/iopoll.h>
#include <linux/reset.h>

#include "cadence-sdhci.h"
#include "sdhci.h"

/* HRS - Host Register Set (specific to Cadence) */
#define   SDHCI_CDNS_HRS04_ACK			BIT(26)
#define   SDHCI_CDNS_HRS04_RD			BIT(25)
#define   SDHCI_CDNS_HRS04_WR			BIT(24)
#define   SDHCI_CDNS_HRS04_RDATA		GENMASK(23, 16)
#define   SDHCI_CDNS_HRS04_WDATA		GENMASK(15, 8)
#define   SDHCI_CDNS_HRS04_ADDR			GENMASK(5, 0)

#define SDHCI_CDNS_HRS06			0x18		/* eMMC control */
#define   SDHCI_CDNS_HRS06_TUNE_UP		BIT(15)
#define   SDHCI_CDNS_HRS06_TUNE			GENMASK(13, 8)
#define   SDHCI_CDNS_HRS06_MODE			GENMASK(2, 0)
#define   SDHCI_CDNS_HRS06_MODE_SD		0x0
#define   SDHCI_CDNS_HRS06_MODE_MMC_SDR		0x2
#define   SDHCI_CDNS_HRS06_MODE_MMC_DDR		0x3
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS200	0x4
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400	0x5
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400ES	0x6

/* SRS - Slot Register Set (SDHCI-compatible) */
#define SDHCI_CDNS_SRS_BASE			0x200

/* PHY */
#define SDHCI_CDNS_PHY_DLY_SD_HS		0x00
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT		0x01
#define SDHCI_CDNS_PHY_DLY_UHS_SDR12		0x02
#define SDHCI_CDNS_PHY_DLY_UHS_SDR25		0x03
#define SDHCI_CDNS_PHY_DLY_UHS_SDR50		0x04
#define SDHCI_CDNS_PHY_DLY_UHS_DDR50		0x05
#define SDHCI_CDNS_PHY_DLY_EMMC_LEGACY		0x06
#define SDHCI_CDNS_PHY_DLY_EMMC_SDR		0x07
#define SDHCI_CDNS_PHY_DLY_EMMC_DDR		0x08
#define SDHCI_CDNS_PHY_DLY_SDCLK		0x0b
#define SDHCI_CDNS_PHY_DLY_HSMMC		0x0c
#define SDHCI_CDNS_PHY_DLY_STROBE		0x0d

#define MAIN_CLOCK_INDEX			0
#define SD_MASTER_CLOCK_INDEX			1

struct sdhci_cdns_phy_cfg {
	const char *property;
	u8 addr;
};

static const struct sdhci_cdns_phy_cfg sdhci_cdns_phy_cfgs[] = {
	{ "cdns,phy-input-delay-sd-highspeed", SDHCI_CDNS_PHY_DLY_SD_HS, },
	{ "cdns,phy-input-delay-legacy", SDHCI_CDNS_PHY_DLY_SD_DEFAULT, },
	{ "cdns,phy-input-delay-sd-uhs-sdr12", SDHCI_CDNS_PHY_DLY_UHS_SDR12, },
	{ "cdns,phy-input-delay-sd-uhs-sdr25", SDHCI_CDNS_PHY_DLY_UHS_SDR25, },
	{ "cdns,phy-input-delay-sd-uhs-sdr50", SDHCI_CDNS_PHY_DLY_UHS_SDR50, },
	{ "cdns,phy-input-delay-sd-uhs-ddr50", SDHCI_CDNS_PHY_DLY_UHS_DDR50, },
	{ "cdns,phy-input-delay-mmc-highspeed", SDHCI_CDNS_PHY_DLY_EMMC_SDR, },
	{ "cdns,phy-input-delay-mmc-ddr", SDHCI_CDNS_PHY_DLY_EMMC_DDR, },
	{ "cdns,phy-dll-delay-sdclk", SDHCI_CDNS_PHY_DLY_SDCLK, },
	{ "cdns,phy-dll-delay-sdclk-hsmmc", SDHCI_CDNS_PHY_DLY_HSMMC, },
	{ "cdns,phy-dll-delay-strobe", SDHCI_CDNS_PHY_DLY_STROBE, },
};

static int sdhci_cdns_write_phy_reg(struct sdhci_cdns_priv *priv,
				    u8 addr, u8 data)
{
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int ret;

	ret = readl_poll_timeout(reg, tmp, !(tmp & SDHCI_CDNS_HRS04_ACK), 10);
	if (ret)
		return ret;

	tmp = FIELD_PREP(SDHCI_CDNS_HRS04_WDATA, data) |
	      FIELD_PREP(SDHCI_CDNS_HRS04_ADDR, addr);
	writel(tmp, reg);

	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, !(tmp & SDHCI_CDNS_HRS04_ACK), 10);

	return ret;
}

static unsigned int sdhci_cdns_phy_param_count(struct device_node *np)
{
	unsigned int count = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++)
		if (of_property_read_bool(np, sdhci_cdns_phy_cfgs[i].property))
			count++;

	return count;
}

static void sdhci_cdns_phy_param_parse(struct device_node *np,
				       struct sdhci_cdns_priv *priv)
{
	struct sdhci_cdns4_phy_param *p = priv->phy_params;
	u32 val;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++) {
		ret = of_property_read_u32(np, sdhci_cdns_phy_cfgs[i].property,
					   &val);
		if (ret)
			continue;

		p->addr = sdhci_cdns_phy_cfgs[i].addr;
		p->data = val;
		p++;
	}
}

static int sdhci_cdns_phy_init(struct sdhci_cdns_priv *priv)
{
	int ret, i;

	for (i = 0; i < priv->nr_phy_params; i++) {
		ret = sdhci_cdns_write_phy_reg(priv, priv->phy_params[i].addr,
					       priv->phy_params[i].data);
		if (ret)
			return ret;
	}

	return 0;
}

static void sdhci_cdns_set_emmc_mode(struct sdhci_cdns_priv *priv, u32 mode)
{
	u32 tmp;

	/* The speed mode for eMMC is selected by HRS06 register */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS06);
	tmp &= ~SDHCI_CDNS_HRS06_MODE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_MODE, mode);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS06);
}

static int sdhci_cdns_init(struct mci_host *host, struct device *dev)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	int ret;

	ret = sdhci_reset(&priv->sdhci, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	sdhci_write8(&priv->sdhci, SDHCI_POWER_CONTROL,
		     SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&priv->sdhci, SDHCI_INT_ENABLE, SDHCI_INT_CMD_COMPLETE |
			SDHCI_INT_XFER_COMPLETE | SDHCI_INT_CARD_INT |
			SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | SDHCI_INT_END_BIT |
			SDHCI_INT_INDEX | SDHCI_INT_DATA_TIMEOUT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_END_BIT | SDHCI_INT_DMA);
	sdhci_write32(&priv->sdhci, SDHCI_SIGNAL_ENABLE, 0);

	sdhci_enable_v4_mode(&priv->sdhci);

	return 0;
}

static void sdhci_cdns_set_ios(struct mci_host *host, struct mci_ios *ios)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	u16 val;

	debug("%s: clock = %u, f_max = %u, bus-width = %d, timing = %02x\n", __func__,
	      ios->clock, host->f_max, ios->bus_width, ios->timing);

	if (ios->clock)
		sdhci_set_clock(&priv->sdhci, ios->clock, priv->sdhci.max_clk);

	sdhci_set_bus_width(&priv->sdhci, ios->bus_width);

	val = sdhci_read8(&priv->sdhci, SDHCI_HOST_CONTROL);

	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(&priv->sdhci, SDHCI_HOST_CONTROL, val);
}

static int sdhci_cdns_send_cmd(struct mci_host *host, struct mci_cmd *cmd)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	int timeout = 10; /* Approx. 10 ms */

	while (sdhci_send_command(&priv->sdhci, cmd) != 0) {
		if (!timeout--) {
			sdhci_dumpregs(&priv->sdhci);
			return -EIO;
		}

		mdelay(1);
	}
	return 0;
}

static int sdhci_cdns_set_tune_val(struct mci_host *host, unsigned int val)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS06;
	u32 tmp;
	int i, ret;

	if (priv->sdhci.version >= SDHCI_SPEC_420)
		return sdhci_cdns6_set_tune_val(host, val);

	if (WARN_ON(!FIELD_FIT(SDHCI_CDNS_HRS06_TUNE, val)))
		return -EINVAL;

	tmp = readl(reg);
	tmp &= ~SDHCI_CDNS_HRS06_TUNE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_TUNE, val);

	/*
	 * Workaround for IP errata:
	 * The IP6116 SD/eMMC PHY design has a timing issue on receive data
	 * path. Send tune request twice.
	 */
	for (i = 0; i < 2; i++) {
		tmp |= SDHCI_CDNS_HRS06_TUNE_UP;
		writel(tmp, reg);

		ret = readl_poll_timeout(reg, tmp,
					 !(tmp & SDHCI_CDNS_HRS06_TUNE_UP), 0);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * In SD mode, software must not use the hardware tuning and instead perform
 * an almost identical procedure to eMMC.
 */
static int sdhci_cdns_execute_tuning(struct mci_host *host, u32 opcode)
{
	int cur_streak = 0;
	int max_streak = 0;
	int end_of_streak = 0;
	int i;
	int ret;

	/*
	 * Do not execute tuning for UHS_SDR50 or UHS_DDR50.
	 * The delay is set by probe, based on the DT properties.
	 */
	if (host->ios.timing != MMC_TIMING_MMC_HS200 &&
	    host->ios.timing != MMC_TIMING_UHS_SDR104) {
		dev_dbg(host->hw_dev, "Tuning skipped (timing: %d)\n",
			host->ios.timing);
		return 0;
	}

	for (i = 0; i < SDHCI_CDNS_MAX_TUNING_LOOP; i++) {
		if (sdhci_cdns_set_tune_val(host, i) ||
		    mmc_send_tuning(host->mci, opcode)) { /* bad */
			cur_streak = 0;
		} else { /* good */
			cur_streak++;
			if (cur_streak > max_streak) {
				max_streak = cur_streak;
				end_of_streak = i;
			}
		}
	}

	if (!max_streak) {
		dev_err(host->hw_dev, "no tuning point found\n");
		return -EIO;
	}

	ret = sdhci_cdns_set_tune_val(host, end_of_streak - max_streak / 2);
	if (ret)
		return ret;

	return 0;
}

static void sdhci_cdns_set_uhs_signaling(struct mci_host *host,
					 unsigned int timing)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	u32 mode;

	switch (timing) {
	case MMC_TIMING_MMC_HS:
		mode = SDHCI_CDNS_HRS06_MODE_MMC_SDR;
		break;
	case MMC_TIMING_MMC_DDR52:
		mode = SDHCI_CDNS_HRS06_MODE_MMC_DDR;
		break;
	case MMC_TIMING_MMC_HS200:
		mode = SDHCI_CDNS_HRS06_MODE_MMC_HS200;
		break;
	case MMC_TIMING_MMC_HS400:
		if (priv->enhanced_strobe)
			mode = SDHCI_CDNS_HRS06_MODE_MMC_HS400ES;
		else
			mode = SDHCI_CDNS_HRS06_MODE_MMC_HS400;
		break;
	default:
		mode = SDHCI_CDNS_HRS06_MODE_SD;
		break;
	}

	sdhci_cdns_set_emmc_mode(priv, mode);

	/* For SD, fall back to the default handler */
	if (mode == SDHCI_CDNS_HRS06_MODE_SD)
		sdhci_set_uhs_signaling(&priv->sdhci, timing);

	/* For host controller V6, set SDHCI and PHY registers for UHS signaling */
	if (priv->sdhci.version >= SDHCI_SPEC_420)
		sdhci_cdns6_phy_adj(host, timing);
}

static const struct mci_ops sdhci_cdns_ops = {
	.set_ios = sdhci_cdns_set_ios,
	.init = sdhci_cdns_init,
	.send_cmd = sdhci_cdns_send_cmd,
	.set_uhs_signaling = sdhci_cdns_set_uhs_signaling,
};

static const struct sdhci_cdns_drv_data sdhci_cdns6_agilex5_drv_data = {
	.ops = &sdhci_cdns_ops,
};

static const struct sdhci_cdns_drv_data sdhci_cdns4_drv_data = {
	.ops = &sdhci_cdns_ops,
};

static int sdhci_cdns4_phy_probe(struct device *dev,
				 struct sdhci_cdns_priv *priv)
{
	sdhci_cdns_phy_param_parse(dev->of_node, priv);

	return sdhci_cdns_phy_init(priv);
}

static struct clk **sdhci_cdns_enable_clocks(struct device *dev, bool is_sd4hc)
{
	struct clk **clks = NULL;

	if (is_sd4hc) {
		/* For SDHCI V4 controllers, enable only the default clock */
		clks = xzalloc(sizeof(struct clk *));
		clks[MAIN_CLOCK_INDEX] = clk_get_enabled(dev, NULL);
		if (IS_ERR(clks[MAIN_CLOCK_INDEX])) {
			dev_err(dev, "Failed to get/enable clock\n");
			return ERR_CAST(clks[MAIN_CLOCK_INDEX]);
		}
	} else {
		clks = xzalloc(sizeof(struct clk *) * 2);

		/* Enable main clock ("biu") */
		clks[MAIN_CLOCK_INDEX] = clk_get_enabled(dev, "biu");
		if (IS_ERR(clks[MAIN_CLOCK_INDEX])) {
			dev_err(dev, "%s: request of Main clock failed (%ld)\n",
				__func__, PTR_ERR(clks[MAIN_CLOCK_INDEX]));
			return ERR_CAST(clks[MAIN_CLOCK_INDEX]);
		}

		/* Enable SD master clock ("ciu") */
		clks[SD_MASTER_CLOCK_INDEX] = clk_get_enabled(dev, "ciu");
		if (IS_ERR(clks[SD_MASTER_CLOCK_INDEX])) {
			dev_err(dev,
				"%s: request of SD master clock failed (%ld)\n",
				__func__, PTR_ERR(clks[SD_MASTER_CLOCK_INDEX]));
			return ERR_CAST(clks[SD_MASTER_CLOCK_INDEX]);
		}
	}
	return clks;
}

static int sdhci_cdns_probe(struct device *dev)
{
	struct sdhci_cdns_priv *priv;
	struct mci_host *mci;
	struct clk **clks;
	struct resource *iores;
	unsigned int nr_phy_params;
	int ret;
	bool is_sd4hc = of_device_is_compatible(dev->device_node, "cdns,sd4hc");

	priv = xzalloc(sizeof(*priv));
	mci = &priv->mci;

	priv->rst = reset_control_get(dev, "sdhc-reset");
	if (IS_ERR(priv->rst)) {
		dev_err(dev, "Invalid reset line 'sdhc-reset'.\n");
		return PTR_ERR(priv->rst);
	}

	priv->softphy_rst = reset_control_get(dev, "softphy-reset");
	if (IS_ERR(priv->softphy_rst)) {
		dev_err(dev, "Invalid reset line 'softphy-reset'.\n");
		return PTR_ERR(priv->softphy_rst);
	}

	priv->sdmmc_ocp_rst = reset_control_get(dev, "sdmmc-ocp");
	if (IS_ERR(priv->sdmmc_ocp_rst)) {
		dev_err(dev, "Invalid reset line 'sdmmc-ocp-reset'.\n");
		return PTR_ERR(priv->sdmmc_ocp_rst);
	}
	reset_control_assert(priv->rst);
	reset_control_deassert(priv->rst);

	reset_control_assert(priv->softphy_rst);
	reset_control_deassert(priv->softphy_rst);

	reset_control_assert(priv->sdmmc_ocp_rst);
	reset_control_deassert(priv->sdmmc_ocp_rst);

	clks = sdhci_cdns_enable_clocks(dev, is_sd4hc);
	if (IS_ERR(clks)) {
		dev_err(dev, "Failed to enable controller clocks: %ld\n", PTR_ERR(clks));
		return PTR_ERR(clks);
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	mci->ops = sdhci_cdns_ops;
	mci->hw_dev = dev;

	nr_phy_params = sdhci_cdns_phy_param_count(dev->of_node);

	priv->biu_clk = clks[MAIN_CLOCK_INDEX];

	priv->nr_phy_params = nr_phy_params;
	priv->hrs_addr = IOMEM(iores->start);
	priv->enhanced_strobe = false;
	priv->sdhci.base = IOMEM(iores->start + SDHCI_CDNS_SRS_BASE);
	priv->sdhci.mci = mci;
	priv->sdhci.max_clk = clk_get_rate(priv->biu_clk);
	priv->sdhci.quirks2 = SDHCI_QUIRK2_BROKEN_HS200;

	dev->priv = priv;

	priv->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	priv->mci.host_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;
	priv->mci.host_caps |= MMC_CAP_MMC_HIGHSPEED_52MHZ;

	priv->mci.f_max = clk_get_rate(priv->biu_clk);
	mci_of_parse(mci);
	priv->mci.f_min = priv->mci.f_max / SDHCI_MAX_DIV_SPEC_300;

	if (is_sd4hc) {
		ret = sdhci_cdns4_phy_probe(dev, priv);
	} else {
		priv->ciu_clk = clks[SD_MASTER_CLOCK_INDEX];
		ret = sdhci_cdns6_phy_probe(mci);
	}
	if (ret)
		goto disable_clk;

	if (IS_ENABLED(CONFIG_MCI_TUNING))
		mci->ops.execute_tuning = sdhci_cdns_execute_tuning;

	ret = sdhci_setup_host(&priv->sdhci);
	if (ret)
		goto disable_clk;

	if (priv->sdhci.caps & SDHCI_CAN_64BIT_V4) {
		dma_set_mask(dev, DMA_BIT_MASK(40));
		dev_dbg(priv->mci.hw_dev, "set DMA mask to 40-bit\n");
	} else {
		dma_set_mask(dev, DMA_BIT_MASK(32));
		dev_dbg(priv->mci.hw_dev, "set DMA mask to 32-bit\n");
	}

	dev_dbg(priv->mci.hw_dev, "host controller version: %u\n",
		priv->sdhci.version);

	ret = mci_register(priv->sdhci.mci);
	if (ret)
		return ret;

	return 0;

disable_clk:
	clk_disable_unprepare(priv->biu_clk);

	return ret;
}

static __maybe_unused struct of_device_id sdhci_cdns_match[] = {
	{
		.compatible = "cdns,sd4hc",
		.data = &sdhci_cdns4_drv_data,
	},
	{
		.compatible = "altr,agilex5-sd6hc",
		.data = &sdhci_cdns6_agilex5_drv_data,
	},
	{ /* sentinel */ }
};

static struct driver sdhci_cdns_driver = {
	.name = "sdhci-cdns",
	.of_compatible = DRV_OF_COMPAT(sdhci_cdns_match),
	.probe = sdhci_cdns_probe,
};
device_platform_driver(sdhci_cdns_driver);
