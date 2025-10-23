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

#include "sdhci.h"

/* HRS - Host Register Set (specific to Cadence) */
#define SDHCI_CDNS_HRS04		0x10		/* PHY access port */
#define SDHCI_CDNS_HRS05        0x14        /* PHY data access port */
#define   SDHCI_CDNS_HRS04_ACK			BIT(26)
#define   SDHCI_CDNS_HRS04_RD			BIT(25)
#define   SDHCI_CDNS_HRS04_WR			BIT(24)
#define   SDHCI_CDNS_HRS04_RDATA		GENMASK(23, 16)
#define   SDHCI_CDNS_HRS04_WDATA		GENMASK(15, 8)
#define   SDHCI_CDNS_HRS04_ADDR			GENMASK(5, 0)

#define SDHCI_CDNS_HRS06		0x18		/* eMMC control */
#define   SDHCI_CDNS_HRS06_TUNE_UP		BIT(15)
#define   SDHCI_CDNS_HRS06_TUNE			GENMASK(13, 8)
#define   SDHCI_CDNS_HRS06_MODE			GENMASK(2, 0)
#define   SDHCI_CDNS_HRS06_MODE_SD		0x0
#define   SDHCI_CDNS_HRS06_MODE_MMC_SDR		0x2
#define   SDHCI_CDNS_HRS06_MODE_MMC_DDR		0x3
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS200	0x4
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400	0x5
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400ES	0x6

/* PHY specific register */
/* HRS register to set after SDMMC reset */
#define SDHCI_CDNS_HRS00	0x0
#define SDHCI_CDNS_HRS07	0x1C        /* IO_DELAY_INFO_REG */
#define SDHCI_CDNS_HRS07_RW_COMPENSATE  GENMASK(20, 16) /* RW_COMPENSATE */
#define SDHCI_CDNS_HRS07_IDELAY_VAL     GENMASK(4, 0)   /* IDELAY_VAL */
/* TODO: check DV dfi_init val=9 */
#define SDHCI_CDNS_HRS07_RW_COMPENSATE_DATA 0x9
/* TODO: check DV dfi_init val=8 ; DDR Mode */
#define SDHCI_CDNS_HRS07_RW_COMPENSATE_DATA_DDR 0x8
#define SDHCI_CDNS_HRS07_IDELAY_VAL_DATA    0x0

#define SDHCI_CDNS_HRS09	0x024
#define SDHCI_CDNS_HRS09_PHY_SW_RESET       BIT(0)  /* PHY_SW_RESET */
#define SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE  BIT(1)  /* PHY_INIT_COMPLETE */
#define SDHCI_CDNS_HRS09_RDDATA_EN  BIT(16) /* RDDATA_EN */
#define SDHCI_CDNS_HRS09_RDCMD_EN   BIT(15) /* RDCMD_EN */
#define SDHCI_CDNS_HRS09_EXTENDED_WR_MODE   BIT(3)  /* EXTENDED_WR_MODE */
#define SDHCI_CDNS_HRS09_EXTENDED_RD_MODE   BIT(2)  /* EXTENDED_RD_MODE */

#define SDHCI_CDNS_HRS10        0x28        /* PHY reset port */
#define SDHCI_CDNS_HRS10_HCSDCLKADJ     GENMASK(19, 16) /* HCSDCLKADJ */
#define SDHCI_CDNS_HRS10_HCSDCLKADJ_DATA    0x0         /* HCSDCLKADJ DATA */
/* HCSDCLKADJ DATA; DDR Mode */
#define SDHCI_CDNS_HRS10_HCSDCLKADJ_DATA_DDR    0x2
#define SDHCI_CDNS_HRS16        0x40        /* CMD_DATA_OUTPUT */

/* SRS - Slot Register Set (SDHCI-compatible) */
#define SDHCI_CDNS_SRS_BASE	0x200
#define SDHCI_CDNS_SRS09	0x224
#define SDHCI_CDNS_SRS10	0x228
#define SDHCI_CDNS_SRS11	0x22c
#define SDHCI_CDNS_SRS12	0x230
#define SDHCI_CDNS_SRS13	0x234
#define SDHCI_CDNS_SRS09_CI	BIT(16)
#define SDHCI_CDNS_SRS13_DATA	0xffffffff
#define SD_HOST_CLK 200000000

/* PHY */
#define SDHCI_CDNS_PHY_DLY_SD_HS	0x00
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT	0x01
#define SDHCI_CDNS_PHY_DLY_UHS_SDR12	0x02
#define SDHCI_CDNS_PHY_DLY_UHS_SDR25	0x03
#define SDHCI_CDNS_PHY_DLY_UHS_SDR50	0x04
#define SDHCI_CDNS_PHY_DLY_UHS_DDR50	0x05
#define SDHCI_CDNS_PHY_DLY_EMMC_LEGACY	0x06
#define SDHCI_CDNS_PHY_DLY_EMMC_SDR	0x07
#define SDHCI_CDNS_PHY_DLY_EMMC_DDR	0x08
#define SDHCI_CDNS_PHY_DLY_SDCLK	0x0b
#define SDHCI_CDNS_PHY_DLY_HSMMC	0x0c
#define SDHCI_CDNS_PHY_DLY_STROBE	0x0d
/* PHY register values */
#define PHY_DQ_TIMING_REG			0x2000
#define PHY_DQS_TIMING_REG			0x2004
#define PHY_GATE_LPBK_CTRL_REG			0x2008
#define PHY_DLL_MASTER_CTRL_REG			0x200C
#define PHY_DLL_SLAVE_CTRL_REG			0x2010
#define PHY_CTRL_REG				0x2080
#define USE_EXT_LPBK_DQS			BIT(22)
#define USE_LPBK_DQS				BIT(21)
#define USE_PHONY_DQS				BIT(20)
#define USE_PHONY_DQS_CMD			BIT(19)
#define SYNC_METHOD				BIT(31)
#define SW_HALF_CYCLE_SHIFT			BIT(28)
#define RD_DEL_SEL				GENMASK(24, 19)
#define RD_DEL_SEL_DATA				0x34
#define GATE_CFG_ALWAYS_ON			BIT(6)
#define UNDERRUN_SUPPRESS			BIT(18)
#define PARAM_DLL_BYPASS_MODE			BIT(23)
#define PARAM_PHASE_DETECT_SEL			GENMASK(22, 20)
#define PARAM_DLL_START_POINT			GENMASK(7, 0)
#define PARAM_PHASE_DETECT_SEL_DATA		0x2
#define PARAM_DLL_START_POINT_DATA		0x4
#define PARAM_DLL_START_POINT_DATA_SDR50	254

#define READ_DQS_CMD_DELAY		GENMASK(31, 24)
#define CLK_WRDQS_DELAY			GENMASK(23, 16)
#define CLK_WR_DELAY			GENMASK(15, 8)
#define READ_DQS_DELAY			GENMASK(7, 0)
#define READ_DQS_CMD_DELAY_DATA		0x0
#define CLK_WRDQS_DELAY_DATA		0x0
#define CLK_WR_DELAY_DATA		0x0
#define READ_DQS_DELAY_DATA		0x0

#define PHONY_DQS_TIMING		GENMASK(9, 4)
#define PHONY_DQS_TIMING_DATA		0x0

#define IO_MASK_ALWAYS_ON		BIT(31)
#define IO_MASK_END			GENMASK(29, 27)
#define IO_MASK_START			GENMASK(26, 24)
#define DATA_SELECT_OE_END		GENMASK(2, 0)
#define IO_MASK_END_DATA		0x5
/* DDR Mode */
#define IO_MASK_END_DATA_DDR		0x2
#define IO_MASK_START_DATA		0x0
#define DATA_SELECT_OE_END_DATA 	0x1

/*
 * The tuned val register is 6 bit-wide, but not the whole of the range is
 * available.  The range 0-42 seems to be available (then 43 wraps around to 0)
 * but I am not quite sure if it is official.  Use only 0 to 39 for safety.
 */
#define SDHCI_CDNS_MAX_TUNING_LOOP	40

struct sdhci_cdns_phy_param {
	u32 addr;
	u32 data;
	u32 offset;
};

struct sdhci_cdns_priv {
	struct mci_host mci;
	struct sdhci sdhci;
	void __iomem *hrs_addr;
	bool enhanced_strobe;
	unsigned int nr_phy_params;
	struct sdhci_cdns_phy_param phy_params[];
};

struct sdhci_cdns_phy_cfg {
	const char *property;
	u32 addr;
	u32 offset;
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
	{ "cdns,phy-use-ext-lpbk-dqs", PHY_DQS_TIMING_REG, 22,},
	{ "cdns,phy-use-lpbk-dqs", PHY_DQS_TIMING_REG, 21,},
	{ "cdns,phy-use-phony-dqs", PHY_DQS_TIMING_REG, 20,},
	{ "cdns,phy-use-phony-dqs-cmd", PHY_DQS_TIMING_REG, 19,},
	{ "cdns,phy-io-mask-always-on", PHY_DQ_TIMING_REG, 31,},
	{ "cdns,phy-io-mask-end", PHY_DQ_TIMING_REG, 27,},
	{ "cdns,phy-io-mask-start", PHY_DQ_TIMING_REG, 24,},
	{ "cdns,phy-data-select-oe-end", PHY_DQ_TIMING_REG, 0,},
	{ "cdns,phy-sync-method", PHY_GATE_LPBK_CTRL_REG, 31,},
	{ "cdns,phy-sw-half-cycle-shift", PHY_GATE_LPBK_CTRL_REG, 28,},
	{ "cdns,phy-rd-del-sel", PHY_GATE_LPBK_CTRL_REG, 19,},
	{ "cdns,phy-underrun-suppress", PHY_GATE_LPBK_CTRL_REG, 18,},
	{ "cdns,phy-gate-cfg-always-on", PHY_GATE_LPBK_CTRL_REG, 6,},
	{ "cdns,phy-param-dll-bypass-mode", PHY_DLL_MASTER_CTRL_REG, 23,},
	{ "cdns,phy-param-phase-detect-sel", PHY_DLL_MASTER_CTRL_REG, 20,},
	{ "cdns,phy-param-dll-start-point", PHY_DLL_MASTER_CTRL_REG, 0,},
	{ "cdns,phy-read-dqs-cmd-delay", PHY_DLL_SLAVE_CTRL_REG, 24,},
	{ "cdns,phy-clk-wrdqs-delay", PHY_DLL_SLAVE_CTRL_REG, 16,},
	{ "cdns,phy-clk-wr-delay", PHY_DLL_SLAVE_CTRL_REG, 8,},
	{ "cdns,phy-read-dqs-delay", PHY_DLL_SLAVE_CTRL_REG, 0,},
	{ "cdns,phy-phony-dqs-timing", PHY_CTRL_REG, 4,},
	{ "cdns,hrs09-rddata-en", SDHCI_CDNS_HRS09, 16,},
	{ "cdns,hrs09-rdcmd-en", SDHCI_CDNS_HRS09, 15,},
	{ "cdns,hrs09-extended-wr-mode", SDHCI_CDNS_HRS09, 3,},
	{ "cdns,hrs09-extended-rd-mode", SDHCI_CDNS_HRS09, 2,},
	{ "cdns,hrs10-hcsdclkadj", SDHCI_CDNS_HRS10, 16,},
	{ "cdns,hrs16-wrdata1-sdclk-dly", SDHCI_CDNS_HRS16, 28,},
	{ "cdns,hrs16-wrdata0-sdclk-dly", SDHCI_CDNS_HRS16, 24,},
	{ "cdns,hrs16-wrcmd1-sdclk-dly", SDHCI_CDNS_HRS16, 20,},
	{ "cdns,hrs16-wrcmd0-sdclk-dly", SDHCI_CDNS_HRS16, 16,},
	{ "cdns,hrs16-wrdata1-dly", SDHCI_CDNS_HRS16, 12,},
	{ "cdns,hrs16-wrdata0-dly", SDHCI_CDNS_HRS16, 8,},
	{ "cdns,hrs16-wrcmd1-dly", SDHCI_CDNS_HRS16, 4,},
	{ "cdns,hrs16-wrcmd0-dly", SDHCI_CDNS_HRS16, 0,},
	{ "cdns,hrs07-rw-compensate", SDHCI_CDNS_HRS07, 16,},
	{ "cdns,hrs07-idelay-val", SDHCI_CDNS_HRS07, 0,},
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

static int sdhci_cdns_write_phy_reg_mask(struct sdhci_cdns_priv *priv,
			u32 addr, u32 data, u32 mask)
{
	u32 tmp;

	tmp = addr;

	/* get PHY address */
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS04);

	/* read current PHY register value, before write */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS05);

	tmp &= ~mask;
	tmp |= data;

	/* write operation */
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS05);

	/* re-read PHY address */
	writel(addr, priv->hrs_addr + SDHCI_CDNS_HRS04);

	/* re-read current PHY register value, check */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS05);

	return 0;
}

static u32 sdhci_cdns_read_phy_reg(struct sdhci_cdns_priv *priv,
			u32 addr)
{
	u32 tmp;

	tmp = addr;

	/* get PHY address */
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS04);

	/* read current PHY register value, before write */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS05);

	return tmp;
}

static int sdhci_cdns_dfi_phy_val(struct sdhci_cdns_priv *priv, u32 reg)
{
	int i;
	u32 tmp;

	tmp = 0;

	for (i = 0; i < priv->nr_phy_params; i++) {
		if (priv->phy_params[i].addr == reg)
			tmp |= priv->phy_params[i].data << priv->phy_params[i].offset;
	}

	return tmp;
}

static int sdhci_cdns_combophy_init_sd_dfi_init(struct sdhci_cdns_priv *priv)
{
	int ret = 0;
	u32 mask = 0x0;
	u32 tmp = 0;

	writel(0x0, priv->hrs_addr + SDHCI_CDNS_SRS11);
	writel(1<<0, priv->hrs_addr + SDHCI_CDNS_HRS00);
	while ((readl(priv->hrs_addr + SDHCI_CDNS_HRS00) & 1<<0) == 1)

	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & ~1;
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	tmp = (1 << 22) | (1 << 21) | (1 << 20) | (1 << 19);
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DQS_TIMING_REG, tmp, tmp);

	tmp = (1 << 31) | (0 << 28) | (52 << 19) | (1 << 18) | (1 << 6);
	mask = SYNC_METHOD | SW_HALF_CYCLE_SHIFT | RD_DEL_SEL | UNDERRUN_SUPPRESS |
			GATE_CFG_ALWAYS_ON;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_GATE_LPBK_CTRL_REG, tmp,
			mask);

	tmp = (1 << 23) | (2 << 20) | (4 << 0);
	mask = PARAM_DLL_BYPASS_MODE | PARAM_PHASE_DETECT_SEL |
			PARAM_DLL_START_POINT;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DLL_MASTER_CTRL_REG, tmp,
			mask);

	tmp = (0 << 24) | (0 << 16) | (0 << 8) | (0 << 0);
	mask = READ_DQS_CMD_DELAY | CLK_WRDQS_DELAY | CLK_WR_DELAY | READ_DQS_DELAY;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DLL_SLAVE_CTRL_REG, tmp,
			mask);

	writel(0x2080, priv->hrs_addr + SDHCI_CDNS_HRS04);
	tmp &= ~(0x3f << 4);
	mask = PHONY_DQS_TIMING;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_CTRL_REG, tmp, mask);

	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) | 1;
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	while (~readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & (1 << 1))

	tmp = sdhci_cdns_read_phy_reg(priv, PHY_DQ_TIMING_REG) & 0x07FFFF8;
	tmp |= (0 << 31) | (0 << 27) | (0 << 24) | (1 << 0);
	mask = IO_MASK_ALWAYS_ON | IO_MASK_END | IO_MASK_START | DATA_SELECT_OE_END;

	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DQ_TIMING_REG, tmp, mask);

	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & 0xFFFE7FF3;

	tmp |= (1 << 16) | (1 << 15) | (1 << 3) | (1 << 2);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS10) & 0xFFF0FFFF;

	tmp |= (0 << 16);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS10);

	tmp = (0 << 28) | (0 << 24) | (0 << 20) | (0 << 16) | (0 << 12) | (1 << 8) |
			(0 << 4) | (1 << 0);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS16);

	tmp = (9 << 16) | (0 << 0);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS07);

	return ret;
}

static int sdhci_cdns_combophy_init_sd_gen(struct sdhci_cdns_priv *priv)
{
	u32 tmp;
	int ret = 0;
	u32 mask = 0x0;

	/* step 1, switch on DLL_RESET */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & ~1;
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	/* step 2, program PHY_DQS_TIMING_REG */
	tmp = sdhci_cdns_dfi_phy_val(priv, PHY_DQS_TIMING_REG);
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DQS_TIMING_REG, tmp, tmp);

	/* step 3, program PHY_GATE_LPBK_CTRL_REG */
	tmp = sdhci_cdns_dfi_phy_val(priv, PHY_GATE_LPBK_CTRL_REG);
	mask = SYNC_METHOD | SW_HALF_CYCLE_SHIFT | RD_DEL_SEL | UNDERRUN_SUPPRESS |
	GATE_CFG_ALWAYS_ON;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_GATE_LPBK_CTRL_REG, tmp,
	mask);

	/* step 4, program PHY_DLL_MASTER_CTRL_REG */
	tmp = sdhci_cdns_dfi_phy_val(priv, PHY_DLL_MASTER_CTRL_REG);
	mask = PARAM_DLL_BYPASS_MODE | PARAM_PHASE_DETECT_SEL |
	PARAM_DLL_START_POINT;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DLL_MASTER_CTRL_REG, tmp,
	mask);

	/* step 5, program PHY_DLL_SLAVE_CTRL_REG */
	tmp = sdhci_cdns_dfi_phy_val(priv, PHY_DLL_SLAVE_CTRL_REG);
	mask = READ_DQS_CMD_DELAY | CLK_WRDQS_DELAY | CLK_WR_DELAY | READ_DQS_DELAY;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DLL_SLAVE_CTRL_REG, tmp,
	mask);

	/* step 7, switch off DLL_RESET */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) | 1;
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	/* step 8, polling PHY_INIT_COMPLETE */
	while (~readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & (1 << 1))
	/* polling for PHY_INIT_COMPLETE bit */

	/* step 9, program PHY_DQ_TIMING_REG */
	tmp = sdhci_cdns_read_phy_reg(priv, PHY_DQ_TIMING_REG) & 0x07FFFF8;
	tmp |= sdhci_cdns_dfi_phy_val(priv, PHY_DQ_TIMING_REG);
	mask = IO_MASK_ALWAYS_ON | IO_MASK_END | IO_MASK_START | DATA_SELECT_OE_END;
	ret = sdhci_cdns_write_phy_reg_mask(priv, PHY_DQ_TIMING_REG, tmp, mask);

	/* step 10, program HRS09, register 42 */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS09) & 0xFFFE7FF3;

	tmp |= sdhci_cdns_dfi_phy_val(priv, SDHCI_CDNS_HRS09);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS09);

	/* step 11, program HRS10, register 43 */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS10) & 0xFFF0FFFF;
	tmp |= sdhci_cdns_dfi_phy_val(priv, SDHCI_CDNS_HRS10);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS10);

	/* step 12, program HRS16, register 48 */
	tmp = sdhci_cdns_dfi_phy_val(priv, SDHCI_CDNS_HRS16);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS16);

	/* step 13, program HRS07, register 40 */
	tmp = sdhci_cdns_dfi_phy_val(priv, SDHCI_CDNS_HRS07);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS07);
	/* end of combophy init */

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
	struct sdhci_cdns_phy_param *p = priv->phy_params;
	u32 val;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++) {
		ret = of_property_read_u32(np, sdhci_cdns_phy_cfgs[i].property,
					   &val);
		if (ret)
			continue;

		p->addr = sdhci_cdns_phy_cfgs[i].addr;
		p->data = val;
		p->offset = sdhci_cdns_phy_cfgs[i].offset;
		p++;
	}
}

static int sdhci_cdns_phy_init(struct sdhci_cdns_priv *priv)
{
	int ret, i;

	for (i = 0; i < priv->nr_phy_params; i++) {
		if (priv->phy_params[i].offset)
			break;
		ret = sdhci_cdns_write_phy_reg(priv, priv->phy_params[i].addr,
					       priv->phy_params[i].data);
		if (ret)
			return ret;
	}

	return 0;
}

static void sdhci_cdns_set_clock(struct mci_host *host, unsigned int clock)
{
	struct sdhci_cdns_priv *priv = host->hw_dev->priv;
	int ret;

	ret = sdhci_cdns_combophy_init_sd_gen(priv);

	sdhci_set_clock(&priv->sdhci, clock, host->f_max);
}

static void sdhci_cdns_set_ios(struct mci_host *host, struct mci_ios *ios)
{
	struct sdhci_cdns_priv *priv = host->hw_dev->priv;

	if (ios->clock)
		sdhci_cdns_set_clock(host, ios->clock);

	sdhci_set_bus_width(&priv->sdhci, ios->bus_width);
}

static const struct mci_ops sdhci_cdns_ops = {
	.set_ios = sdhci_cdns_set_ios,
};

static int sdhci_cdns_probe(struct device *dev)
{
	struct sdhci_cdns_priv *priv;
	struct mci_host *mci;
	struct clk *clk;
	struct resource *iores;
	struct reset_control *rst;
	unsigned int nr_phy_params;
	int ret;

	priv = xzalloc(sizeof(*priv));
	mci = &priv->mci;

	clk = clk_get(dev, "biu");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	rst = reset_control_get(dev, "reset");
	if (IS_ERR(rst)) {
		dev_err(dev, "Invalid reset line 'reset'.\n");
		return PTR_ERR(rst);
	}
	reset_control_deassert(rst);
	ret = clk_prepare_enable(clk);
	if (ret)
		return ret;

	mci->ops = sdhci_cdns_ops;
	mci->hw_dev = dev;

	nr_phy_params = sdhci_cdns_phy_param_count(dev->of_node);

	priv->nr_phy_params = nr_phy_params;
	priv->hrs_addr = IOMEM(iores->start);
	priv->enhanced_strobe = false;
	priv->sdhci.base = IOMEM(iores->start + SDHCI_CDNS_SRS_BASE);
	priv->sdhci.quirks2 = SDHCI_QUIRK2_40_BIT_DMA_MASK;
	priv->sdhci.mci = mci;

	mci_of_parse(mci);

	sdhci_cdns_phy_param_parse(dev->of_node, priv);

	ret = sdhci_cdns_phy_init(priv);
	if (ret)
		goto disable_clk;

	ret = sdhci_cdns_combophy_init_sd_dfi_init(priv);
	if (ret)
		goto disable_clk;

	dev->priv = priv;

	ret = sdhci_setup_host(&priv->sdhci);
	if (ret)
		goto disable_clk;

	return 0;

disable_clk:
	clk_disable_unprepare(clk);

	return ret;
}

static __maybe_unused struct of_device_id sdhci_cdns_match[] = {
	{
		.compatible = "cdns,sd4hc"
	},
	{
		.compatible = "intel,agilex5-sd4hc",
	},
	{ /* sentinel */ }
};

static struct driver sdhci_cdns_driver = {
	.name = "sdhci-cdns",
	.of_compatible = DRV_OF_COMPAT(sdhci_cdns_match),
	.probe = sdhci_cdns_probe,
};
device_platform_driver(sdhci_cdns_driver);
