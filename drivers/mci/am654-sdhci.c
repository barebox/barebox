// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Texas Instruments' K3 SD Host Controller Interface
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <mci.h>
#include <clock.h>
#include <errno.h>
#include <io.h>
#include <regmap.h>
#include <linux/err.h>
#include <linux/clk.h>
#include "sdhci.h"

/* CTL_CFG Registers */
#define CTL_CFG_2		0x14

#define SLOTTYPE_MASK		GENMASK(31, 30)
#define SLOTTYPE_EMBEDDED	BIT(30)

/* PHY Registers */
#define PHY_CTRL1	0x100
#define PHY_CTRL2	0x104
#define PHY_CTRL3	0x108
#define PHY_CTRL4	0x10C
#define PHY_CTRL5	0x110
#define PHY_CTRL6	0x114
#define PHY_STAT1	0x130
#define PHY_STAT2	0x134

#define IOMUX_ENABLE_SHIFT	31
#define IOMUX_ENABLE_MASK	BIT(IOMUX_ENABLE_SHIFT)
#define OTAPDLYENA_SHIFT	20
#define OTAPDLYENA_MASK		BIT(OTAPDLYENA_SHIFT)
#define OTAPDLYSEL_SHIFT	12
#define OTAPDLYSEL_MASK		GENMASK(15, 12)
#define STRBSEL_SHIFT		24
#define STRBSEL_4BIT_MASK	GENMASK(27, 24)
#define STRBSEL_8BIT_MASK	GENMASK(31, 24)
#define SEL50_SHIFT		8
#define SEL50_MASK		BIT(SEL50_SHIFT)
#define SEL100_SHIFT		9
#define SEL100_MASK		BIT(SEL100_SHIFT)
#define FREQSEL_SHIFT		8
#define FREQSEL_MASK		GENMASK(10, 8)
#define CLKBUFSEL_SHIFT		0
#define CLKBUFSEL_MASK		GENMASK(2, 0)
#define DLL_TRIM_ICP_SHIFT	4
#define DLL_TRIM_ICP_MASK	GENMASK(7, 4)
#define DR_TY_SHIFT		20
#define DR_TY_MASK		GENMASK(22, 20)
#define ENDLL_SHIFT		1
#define ENDLL_MASK		BIT(ENDLL_SHIFT)
#define DLLRDY_SHIFT		0
#define DLLRDY_MASK		BIT(DLLRDY_SHIFT)
#define PDB_SHIFT		0
#define PDB_MASK		BIT(PDB_SHIFT)
#define CALDONE_SHIFT		1
#define CALDONE_MASK		BIT(CALDONE_SHIFT)
#define RETRIM_SHIFT		17
#define RETRIM_MASK		BIT(RETRIM_SHIFT)
#define SELDLYTXCLK_SHIFT	17
#define SELDLYTXCLK_MASK	BIT(SELDLYTXCLK_SHIFT)
#define SELDLYRXCLK_SHIFT	16
#define SELDLYRXCLK_MASK	BIT(SELDLYRXCLK_SHIFT)
#define ITAPDLYSEL_SHIFT	0
#define ITAPDLYSEL_MASK		GENMASK(4, 0)
#define ITAPDLYENA_SHIFT	8
#define ITAPDLYENA_MASK		BIT(ITAPDLYENA_SHIFT)
#define ITAPCHGWIN_SHIFT	9
#define ITAPCHGWIN_MASK		BIT(ITAPCHGWIN_SHIFT)

#define DRIVER_STRENGTH_50_OHM	0x0
#define DRIVER_STRENGTH_33_OHM	0x1
#define DRIVER_STRENGTH_66_OHM	0x2
#define DRIVER_STRENGTH_100_OHM	0x3
#define DRIVER_STRENGTH_40_OHM	0x4

#define AM654_SDHCI_MIN_FREQ	400000
#define CLOCK_TOO_SLOW_HZ	50000000

#define MMC_CAP2_HS200 0
#define MMC_CAP2_HS400 0
#define MMC_CAP_UHS_SDR104 0
#define MMC_CAP_UHS_SDR12 0
#define MMC_CAP_UHS_DDR50 0
#define MMC_CAP_UHS_SDR25 0
#define MMC_CAP_UHS_SDR50 0

struct timing_data {
	const char *otap_binding;
	const char *itap_binding;
	u32 capability;
};

static const struct timing_data td[] = {
	[MMC_TIMING_LEGACY]	= {
		"ti,otap-del-sel-legacy",
		"ti,itap-del-sel-legacy",
		0
	},
	[MMC_TIMING_MMC_HS]	= {
		"ti,otap-del-sel-mmc-hs",
		"ti,itap-del-sel-mms-hs",
		MMC_CAP_MMC_HIGHSPEED
	},
	[MMC_TIMING_SD_HS]	= {
		"ti,otap-del-sel-sd-hs",
		"ti,itap-del-sel-sd-hs",
		MMC_CAP_SD_HIGHSPEED
	},
	[MMC_TIMING_UHS_SDR12]	= {
		"ti,otap-del-sel-sdr12",
		"ti,itap-del-sel-sdr12",
		MMC_CAP_UHS_SDR12
	},
	[MMC_TIMING_UHS_SDR25]	= {
		"ti,otap-del-sel-sdr25",
		"ti,itap-del-sel-sdr25",
		MMC_CAP_UHS_SDR25
	},
	[MMC_TIMING_UHS_SDR50]	= {
		"ti,otap-del-sel-sdr50",
		NULL,
		MMC_CAP_UHS_SDR50
	},
	[MMC_TIMING_UHS_SDR104]	= {
		"ti,otap-del-sel-sdr104",
		NULL,
		MMC_CAP_UHS_SDR104
	},
	[MMC_TIMING_UHS_DDR50]	= {
		"ti,otap-del-sel-ddr50",
		NULL,
		MMC_CAP_UHS_DDR50
	},
	[MMC_TIMING_MMC_DDR52]	= {
		"ti,otap-del-sel-ddr52",
		"ti,itap-del-sel-ddr52",
		MMC_CAP_DDR
	},
	[MMC_TIMING_MMC_HS200]	= {
		"ti,otap-del-sel-hs200",
		NULL,
		MMC_CAP2_HS200
	},
	[MMC_TIMING_MMC_HS400]	= {
		"ti,otap-del-sel-hs400",
		NULL,
		MMC_CAP2_HS400
	},
};

struct am654_sdhci_plat {
	struct sdhci sdhci;
	struct mci_host mci;
	struct clk *clk;
	struct clk *clk_ahb;
	const struct am654_driver_data *soc_data;
	bool fails_without_test_cd;

	struct regmap *base;
	bool non_removable;
	u32 otap_del_sel[ARRAY_SIZE(td)];
	u32 itap_del_sel[ARRAY_SIZE(td)];
	u32 trm_icp;
	u32 drv_strength;
	u32 strb_sel;
	u32 clkbuf_sel;
#define DLL_PRESENT	BIT(0)
#define IOMUX_PRESENT	BIT(1)
#define FREQSEL_2_BIT	BIT(2)
#define STRBSEL_4_BIT	BIT(3)
#define DLL_CALIB	BIT(4)
};

struct am654_driver_data {
	int (*set_ios_post)(struct am654_sdhci_plat *plat, struct mci_ios *ios);
	u32 flags;
};

static int am654_sdhci_setup_dll(struct am654_sdhci_plat *plat,
				 unsigned int speed)
{
	int sel50, sel100, freqsel;
	u32 mask, val;
	int ret;

	/* Disable delay chain mode */
	regmap_update_bits(plat->base, PHY_CTRL5,
			   SELDLYTXCLK_MASK | SELDLYRXCLK_MASK, 0);

	if (plat->soc_data->flags & FREQSEL_2_BIT) {
		switch (speed) {
		case 200000000:
			sel50 = 0;
			sel100 = 0;
			break;
		case 100000000:
			sel50 = 0;
			sel100 = 1;
			break;
		default:
			sel50 = 1;
			sel100 = 0;
		}

		/* Configure PHY DLL frequency */
		mask = SEL50_MASK | SEL100_MASK;
		val = (sel50 << SEL50_SHIFT) | (sel100 << SEL100_SHIFT);
		regmap_update_bits(plat->base, PHY_CTRL5, mask, val);
	} else {
		switch (speed) {
		case 200000000:
			freqsel = 0x0;
			break;
		default:
			freqsel = 0x4;
		}
		regmap_update_bits(plat->base, PHY_CTRL5, FREQSEL_MASK,
				   freqsel << FREQSEL_SHIFT);
	}

	/* Configure DLL TRIM */
	mask = DLL_TRIM_ICP_MASK;
	val = plat->trm_icp << DLL_TRIM_ICP_SHIFT;

	/* Configure DLL driver strength */
	mask |= DR_TY_MASK;
	val |= plat->drv_strength << DR_TY_SHIFT;
	regmap_update_bits(plat->base, PHY_CTRL1, mask, val);

	/* Enable DLL */
	regmap_update_bits(plat->base, PHY_CTRL1, ENDLL_MASK,
			   0x1 << ENDLL_SHIFT);
	/*
	 * Poll for DLL ready. Use a one second timeout.
	 * Works in all experiments done so far
	 */
	ret = regmap_read_poll_timeout(plat->base, PHY_STAT1, val,
				       val & DLLRDY_MASK, 1000000);

	return ret;
}

static void am654_sdhci_write_itapdly(struct am654_sdhci_plat *plat,
				      u32 itapdly)
{
	/* Set ITAPCHGWIN before writing to ITAPDLY */
	regmap_update_bits(plat->base, PHY_CTRL4, ITAPCHGWIN_MASK,
			   1 << ITAPCHGWIN_SHIFT);
	regmap_update_bits(plat->base, PHY_CTRL4, ITAPDLYSEL_MASK,
			   itapdly << ITAPDLYSEL_SHIFT);
	regmap_update_bits(plat->base, PHY_CTRL4, ITAPCHGWIN_MASK, 0);
}

static void am654_sdhci_setup_delay_chain(struct am654_sdhci_plat *plat,
					  int mode)
{
	u32 mask, val;

	val = 1 << SELDLYTXCLK_SHIFT | 1 << SELDLYRXCLK_SHIFT;
	mask = SELDLYTXCLK_MASK | SELDLYRXCLK_MASK;
	regmap_update_bits(plat->base, PHY_CTRL5, mask, val);

	am654_sdhci_write_itapdly(plat, plat->itap_del_sel[mode]);
}

static int am654_sdhci_set_ios_post(struct am654_sdhci_plat *plat, struct mci_ios *ios)
{
	unsigned int speed = ios->clock;
	int mode = ios->timing;
	u32 otap_del_sel;
	u32 mask, val;
	int ret;

	/* Reset SD Clock Enable */
	val = sdhci_read16(&plat->sdhci, SDHCI_CLOCK_CONTROL);
	val &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_write16(&plat->sdhci, SDHCI_CLOCK_CONTROL, val);

	regmap_update_bits(plat->base, PHY_CTRL1, ENDLL_MASK, 0);

	/* restart clock */
	sdhci_set_clock(&plat->sdhci, speed, clk_get_rate(plat->clk));

	/* switch phy back on */
	otap_del_sel = plat->otap_del_sel[mode];
	mask = OTAPDLYENA_MASK | OTAPDLYSEL_MASK;
	val = (1 << OTAPDLYENA_SHIFT) |
	      (otap_del_sel << OTAPDLYSEL_SHIFT);

	/* Write to STRBSEL for HS400 speed mode */
	if (mode == MMC_TIMING_MMC_HS400) {
		if (plat->soc_data->flags & STRBSEL_4_BIT)
			mask |= STRBSEL_4BIT_MASK;
		else
			mask |= STRBSEL_8BIT_MASK;

		val |= plat->strb_sel << STRBSEL_SHIFT;
	}

	regmap_update_bits(plat->base, PHY_CTRL4, mask, val);

	if (mode > MMC_TIMING_UHS_SDR25 && speed >= CLOCK_TOO_SLOW_HZ) {
		ret = am654_sdhci_setup_dll(plat, speed);
		if (ret)
			return ret;
	} else {
		am654_sdhci_setup_delay_chain(plat, mode);
	}

	regmap_update_bits(plat->base, PHY_CTRL5, CLKBUFSEL_MASK,
			   plat->clkbuf_sel);

	return 0;
}

static int am654_sdhci_init(struct mci_host *mci, struct device *dev)
{
	struct am654_sdhci_plat *plat = container_of(mci, struct am654_sdhci_plat, mci);
	u32 ctl_cfg_2 = 0;
	u32 mask, val;
	int ret;

	ret = sdhci_reset(&plat->sdhci, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	if (plat->fails_without_test_cd) {
		val = sdhci_read8(&plat->sdhci, SDHCI_HOST_CONTROL);
		val |= SDHCI_CTRL_CDTEST_INS | SDHCI_CTRL_CDTEST_EN;
		sdhci_write8(&plat->sdhci, SDHCI_HOST_CONTROL, val);
	}

	sdhci_write8(&plat->sdhci, SDHCI_POWER_CONTROL,
				SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&plat->sdhci, SDHCI_INT_ENABLE, ~0);
	sdhci_write32(&plat->sdhci, SDHCI_SIGNAL_ENABLE, 0x00);

	/* Reset OTAP to default value */
	mask = OTAPDLYENA_MASK | OTAPDLYSEL_MASK;
	regmap_update_bits(plat->base, PHY_CTRL4, mask, 0x0);

	if (plat->soc_data->flags & DLL_CALIB) {
		regmap_read(plat->base, PHY_STAT1, &val);
		if (~val & CALDONE_MASK) {
			/* Calibrate IO lines */
			regmap_update_bits(plat->base, PHY_CTRL1, PDB_MASK,
					   PDB_MASK);
			ret = regmap_read_poll_timeout(plat->base, PHY_STAT1,
						       val, val & CALDONE_MASK,
						       20);
			if (ret)
				return ret;
		}
	}

	/* Enable pins by setting IO mux to 0 */
	if (plat->soc_data->flags & IOMUX_PRESENT)
		regmap_update_bits(plat->base, PHY_CTRL1, IOMUX_ENABLE_MASK, 0);

	/* Set slot type based on SD or eMMC */
	if (plat->non_removable)
		ctl_cfg_2 = SLOTTYPE_EMBEDDED;

	regmap_update_bits(plat->base, CTL_CFG_2, SLOTTYPE_MASK, ctl_cfg_2);

	return 0;
}

const struct am654_driver_data am654_drv_data = {
	.flags = DLL_PRESENT | IOMUX_PRESENT | FREQSEL_2_BIT | STRBSEL_4_BIT,
};

const struct am654_driver_data am654_sr1_drv_data = {
	.flags = IOMUX_PRESENT | FREQSEL_2_BIT | DLL_PRESENT | DLL_CALIB |
		 STRBSEL_4_BIT,
};

const struct am654_driver_data j721e_8bit_drv_data = {
	.flags = DLL_PRESENT | DLL_CALIB,
};

static int j721e_4bit_sdhci_set_ios_post(struct am654_sdhci_plat *plat, struct mci_ios *ios)
{
	u32 otap_del_sel, mask, val;

	otap_del_sel = plat->otap_del_sel[ios->timing];
	mask = OTAPDLYENA_MASK | OTAPDLYSEL_MASK;
	val = (1 << OTAPDLYENA_SHIFT) | (otap_del_sel << OTAPDLYSEL_SHIFT);
	regmap_update_bits(plat->base, PHY_CTRL4, mask, val);

	regmap_update_bits(plat->base, PHY_CTRL5, CLKBUFSEL_MASK,
			   plat->clkbuf_sel);

	return 0;
}

const struct am654_driver_data j721e_4bit_drv_data = {
	.set_ios_post		= &j721e_4bit_sdhci_set_ios_post,
	.flags = IOMUX_PRESENT,
};

static const struct am654_driver_data sdhci_am64_8bit_drvdata = {
	.set_ios_post		= &am654_sdhci_set_ios_post,
	.flags = DLL_PRESENT | DLL_CALIB,
};

static const struct am654_driver_data sdhci_am64_4bit_drvdata = {
	.set_ios_post		= &j721e_4bit_sdhci_set_ios_post,
	.flags = IOMUX_PRESENT,
};

static int sdhci_am654_get_otap_delay(struct am654_sdhci_plat *plat)
{
	struct device *dev = plat->mci.hw_dev;
	struct device_node *np = dev->of_node;
	int ret;
	int i;

	/* ti,otap-del-sel-legacy is mandatory */
	ret = of_property_read_u32(np, "ti,otap-del-sel-legacy",
			   &plat->otap_del_sel[0]);
	if (ret)
		return ret;
	/*
	 * Remove the corresponding capability if an otap-del-sel
	 * value is not found
	 */
	for (i = MMC_TIMING_MMC_HS; i <= MMC_TIMING_MMC_HS400; i++) {
		ret = of_property_read_u32(np, td[i].otap_binding,
				   &plat->otap_del_sel[i]);
		if (ret) {
			dev_dbg(dev, "Couldn't find %s\n", td[i].otap_binding);
			/*
			 * Remove the corresponding capability
			 * if an otap-del-sel value is not found
			 */
			plat->mci.host_caps &= ~td[i].capability;
		}

		if (td[i].itap_binding)
			of_property_read_u32(np, td[i].itap_binding,
				     &plat->itap_del_sel[i]);
	}

	return 0;
}

static void print_error(struct am654_sdhci_plat *host, int cmdidx)
{
	dev_dbg(host->mci.hw_dev,
		"error while transfering data for command %d\n", cmdidx);
	dev_dbg(host->mci.hw_dev, "state = 0x%08x , interrupt = 0x%08x\n",
		sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE),
		sdhci_read32(&host->sdhci, SDHCI_INT_NORMAL_STATUS));
}

static int am654_sdhci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				struct mci_data *data)
{
	struct am654_sdhci_plat *host = container_of(mci, struct am654_sdhci_plat, mci);
	u32 command, xfer;
	int ret;
	dma_addr_t dma;

	ret = sdhci_wait_idle_data(&host->sdhci, cmd);
	if (ret)
		return ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);

	sdhci_setup_data_dma(&host->sdhci, data, &dma);

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				dma == SDHCI_NO_DMA ? false : true,
				&command, &xfer);

	sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	ret = sdhci_wait_for_done(&host->sdhci, SDHCI_INT_CMD_COMPLETE);
	if (ret)
		goto error;

	sdhci_read_response(&host->sdhci, cmd);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, SDHCI_INT_CMD_COMPLETE);

	ret = sdhci_transfer_data_dma(&host->sdhci, data, dma);

error:
	if (ret) {
		print_error(host, cmd->cmdidx);
		sdhci_reset(&host->sdhci, SDHCI_RESET_CMD);
		sdhci_reset(&host->sdhci, SDHCI_RESET_DATA);
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	return ret;
}

static void am654_sdhci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct am654_sdhci_plat *plat = container_of(mci, struct am654_sdhci_plat, mci);
	u32 val;

	if (ios->clock)
		sdhci_set_clock(&plat->sdhci, ios->clock, plat->sdhci.max_clk);

	sdhci_set_bus_width(&plat->sdhci, ios->bus_width);

	val = sdhci_read8(&plat->sdhci, SDHCI_HOST_CONTROL);

	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(&plat->sdhci, SDHCI_HOST_CONTROL, val);

	plat->soc_data->set_ios_post(plat, ios);
}

static const struct regmap_config regmap_config = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
	.max_register	= 0x400,
};

static const struct mci_ops am654_sdhci_ops = {
	.send_cmd = am654_sdhci_send_cmd,
	.set_ios = am654_sdhci_set_ios,
	.init = am654_sdhci_init,
};

static int am654_sdhci_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct am654_sdhci_plat *plat;
	struct mci_host *mci;
	struct resource *iores;
	u32 drv_strength;
	int ret;

	plat = xzalloc(sizeof(*plat));

	ret = dev_get_drvdata(dev, (const void **)&plat->soc_data);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	plat->sdhci.base = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	plat->base = regmap_init_mmio(dev, IOMEM(iores->start), &regmap_config);
        if (IS_ERR(plat->base)) {
                dev_err(dev, "failed to init regmap: %ld\n",
                                PTR_ERR(plat->base));
                return PTR_ERR(plat->base);
        }

	plat->clk = clk_get(dev, "clk_xin");
	if (IS_ERR(plat->clk)) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clk_enable(plat->clk);

	plat->clk_ahb = clk_get(dev, "clk_ahb");
	if (IS_ERR(plat->clk_ahb)) {
		dev_err(dev, "failed to get ahb clock\n");
		return ret;
	}

	clk_enable(plat->clk_ahb);

	mci = &plat->mci;
	mci->f_max = clk_get_rate(plat->clk);
	mci->f_min = 50000000 / 256;

	if (plat->soc_data->flags & DLL_PRESENT) {
		ret = of_property_read_u32(np, "ti,trm-icp", &plat->trm_icp);
		if (ret)
			return ret;

		ret = of_property_read_u32(np, "ti,driver-strength-ohm",
				   &drv_strength);
		if (ret)
			return ret;

		switch (drv_strength) {
		case 50:
			plat->drv_strength = DRIVER_STRENGTH_50_OHM;
			break;
		case 33:
			plat->drv_strength = DRIVER_STRENGTH_33_OHM;
			break;
		case 66:
			plat->drv_strength = DRIVER_STRENGTH_66_OHM;
			break;
		case 100:
			plat->drv_strength = DRIVER_STRENGTH_100_OHM;
			break;
		case 40:
			plat->drv_strength = DRIVER_STRENGTH_40_OHM;
			break;
		default:
			dev_err(dev, "Invalid driver strength\n");
			return -EINVAL;
		}
	}

	mci->ops = am654_sdhci_ops;
	mci->hw_dev = dev;

	of_property_read_u32(np, "ti,strobe-sel", &plat->strb_sel);
	of_property_read_u32(np, "ti,clkbuf-sel", &plat->clkbuf_sel);

	plat->fails_without_test_cd = of_property_read_bool(np, "ti,fails-without-test-cd");

	mci_of_parse(&plat->mci);

	ret = sdhci_am654_get_otap_delay(plat);
	if (ret)
		return ret;

	plat->sdhci.mci = mci;
	sdhci_setup_host(&plat->sdhci);

	dev->priv = plat;

	return mci_register(&plat->mci);
}

static const struct of_device_id am654_sdhci_ids[] = {
	{
		.compatible = "ti,am654-sdhci-5.1",
		.data = &am654_drv_data,
	}, {
		.compatible = "ti,j721e-sdhci-8bit",
		.data = &j721e_8bit_drv_data,
	}, {
		.compatible = "ti,j721e-sdhci-4bit",
		.data = &j721e_4bit_drv_data,
	}, {
		.compatible = "ti,am64-sdhci-8bit",
		.data = &sdhci_am64_8bit_drvdata,
	}, {
		.compatible = "ti,am64-sdhci-4bit",
		.data = &sdhci_am64_4bit_drvdata,
	}, {
		.compatible = "ti,am62-sdhci",
		.data = &sdhci_am64_4bit_drvdata,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, am654_sdhci_ids);

static struct driver am654_sdhc_driver = {
        .name = "am654-sdhci",
        .probe = am654_sdhci_probe,
        .of_compatible = DRV_OF_COMPAT(am654_sdhci_ids),
};
device_platform_driver(am654_sdhc_driver);
