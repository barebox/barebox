// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * PHY support for Cadence version 6 SDHCI
 *
 * Copyright (C) 2025 Altera Corporation
 *   Author: Tanmay Kathpalia <tanmay.kathpalia@altera.com>
 */

#include "cadence-sdhci.h"

/* IO Delay Information */
#define SDHCI_CDNS_HRS07			0X1C

/* PHY Control and Status */
#define SDHCI_CDNS_HRS09			0x24
#define   SDHCI_CDNS_HRS09_RDDATA_EN		BIT(16)
#define   SDHCI_CDNS_HRS09_RDCMD_EN		BIT(15)
#define   SDHCI_CDNS_HRS09_EXTENDED_WR_MODE	BIT(3)
#define   SDHCI_CDNS_HRS09_EXTENDED_RD_MODE	BIT(2)
#define   SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE	BIT(1)
#define   SDHCI_CDNS_HRS09_PHY_SW_RESET		BIT(0)

/* SDCLK start point adjustment */
#define SDHCI_CDNS_HRS10			0x28

/* eMMC Control */
#define SDHCI_CDNS_HRS11			0x2C
#define   SDHCI_CDNS_HRS11_EMMC_RST		BIT(0)	/* eMMC reset */

/* CMD/DAT output delay */
#define SDHCI_CDNS_HRS16			0x40

/* PHY Special Function Registers */
/* DQ timing */
#define PHY_DQ_TIMING_REG_ADDR			0x2000

/* DQS timing */
#define PHY_DQS_TIMING_REG_ADDR			0x2004

/* Gate and loopback control */
#define PHY_GATE_LPBK_CTRL_REG_ADDR		0x2008

/* Master DLL logic */
#define PHY_DLL_MASTER_CTRL_REG_ADDR		0x200C

/* Slave DLL logic */
#define PHY_DLL_SLAVE_CTRL_REG_ADDR			0x2010
#define   PHY_DLL_SLAVE_CTRL_REG_READ_DQS_CMD_DELAY	GENMASK(31, 24)
#define   PHY_DLL_SLAVE_CTRL_REG_READ_DQS_DELAY		GENMASK(7, 0)

/* Global control settings */
#define PHY_CTRL_REG_ADDR			0x2080

/*
 * Supports MMC_TIMING_LEGACY to MMC_TIMING_MMC_HS400 and additionally
 * MMC_TIMING_MMC_HS400ES, as defined in include/linux/mmc/host.h.
 */
#define MAX_TIMING_MODES			12

/* Index register configuration arrays for Cadence SDHCI V6 controller and PHY setup */
enum sdhci_cdns6_reg_index {
	REG_CFG_HRS07 = 0,
	REG_CFG_HRS09,
	REG_CFG_HRS10,
	REG_CFG_HRS16,
	REG_CFG_PHY_DQS,
	REG_CFG_PHY_GATE_LPBK_CTRL,
	REG_CFG_PHY_DLL_MASTER_CTRL,
	REG_CFG_PHY_DLL_SLAVE_CTRL,
	REG_CFG_PHY_DQ,
	REG_CFG_MAX
};

/**
 * struct sdhci_cdns6_ctrl_cfg - Controller/PHY register property and value pair
 * @property: Device tree property name for the register configuration
 * @value: Value to be programmed into the register
 *
 * Store register configuration values parsed from the device tree
 * for Cadence SDHCI V6 controller and PHY initialization.
 */
struct sdhci_cdns6_ctrl_cfg {
	const char *property;
	u32 value;
};

static const struct sdhci_cdns6_ctrl_cfg reg_cfg[REG_CFG_MAX][MAX_TIMING_MODES] = {
	/* [0] SD Host Controller: HRS07 */
	{
		{ "cdns,ctrl-hrs07-timing-delay-default", 0x00090000 }, // MMC legacy or SD default
		{ "cdns,ctrl-hrs07-timing-delay-mmc-hs", 0x00090000 }, // MMC High Speed
		{ "cdns,ctrl-hrs07-timing-delay-sd-hs", 0x000A0001 }, // SD High Speed
		{ "cdns,ctrl-hrs07-timing-delay-sd-sdr12", 0x000A0001 }, // SD UHS1 SDR12
		{ "cdns,ctrl-hrs07-timing-delay-sd-sdr25", 0x000A0001 }, // SD UHS1 SDR25
		{ "cdns,ctrl-hrs07-timing-delay-sd-sdr50", 0x00090005 }, // SD UHS1 SDR50
		{ "cdns,ctrl-hrs07-timing-delay-sd-sdr104", 0x00090005 }, // SD UHS1 SDR104
		{ "cdns,ctrl-hrs07-timing-delay-sd-ddr50", 0x00090001 }, // SD UHS1 DDR50
		{ "cdns,ctrl-hrs07-timing-delay-mmc-ddr52", 0x00090001 }, // MMC DDR52
		{ "cdns,ctrl-hrs07-timing-delay-mmc-hs200", 0x00090000 }, // MMC HS200
		{ "cdns,ctrl-hrs07-timing-delay-mmc-hs400", 0x00090001 }, // MMC HS400
		{ "cdns,ctrl-hrs07-timing-delay-mmc-hs400es", 0x00090001 }, // MMC HS400ES
	},
	/* [1] SD Host Controller: HRS09 */
	{
		{ "cdns,ctrl-hrs09-timing-delay-default", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-mmc-hs", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-sd-hs", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-sd-sdr12", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-sd-sdr25", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-sd-sdr50", 0xf1c1800c },
		{ "cdns,ctrl-hrs09-timing-delay-sd-sdr104", 0xf1c18000 },
		{ "cdns,ctrl-hrs09-timing-delay-sd-ddr50", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-mmc-ddr52", 0x0001800C },
		{ "cdns,ctrl-hrs09-timing-delay-mmc-hs200", 0xf1c18000 },
		{ "cdns,ctrl-hrs09-timing-delay-mmc-hs400", 0xf1c18000 },
		{ "cdns,ctrl-hrs09-timing-delay-mmc-hs400es", 0xf1c18000 },
	},
	/* [2] SD Host Controller: HRS10 */
	{
		{ "cdns,ctrl-hrs10-timing-delay-default", 0x00020000 },
		{ "cdns,ctrl-hrs10-timing-delay-mmc-hs", 0x00020000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-hs", 0x00030000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-sdr12", 0x00030000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-sdr25", 0x00030000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-sdr50", 0x00020000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-sdr104", 0x00090000 },
		{ "cdns,ctrl-hrs10-timing-delay-sd-ddr50", 0x00020000 },
		{ "cdns,ctrl-hrs10-timing-delay-mmc-ddr52", 0x00020000 },
		{ "cdns,ctrl-hrs10-timing-delay-mmc-hs200", 0x00090000 },
		{ "cdns,ctrl-hrs10-timing-delay-mmc-hs400", 0x00080000 },
		{ "cdns,ctrl-hrs10-timing-delay-mmc-hs400es", 0x00080000 },
	},
	/* [3] SD Host Controller: HRS16 */
	{
		{ "cdns,ctrl-hrs16-timing-delay-default", 0x00000000 },
		{ "cdns,ctrl-hrs16-timing-delay-mmc-hs", 0x0000010a },
		{ "cdns,ctrl-hrs16-timing-delay-sd-hs", 0x00000101 },
		{ "cdns,ctrl-hrs16-timing-delay-sd-sdr12", 0x00000000 },
		{ "cdns,ctrl-hrs16-timing-delay-sd-sdr25", 0x00000101 },
		{ "cdns,ctrl-hrs16-timing-delay-sd-sdr50", 0x00000101 },
		{ "cdns,ctrl-hrs16-timing-delay-sd-sdr104", 0x00000101 },
		{ "cdns,ctrl-hrs16-timing-delay-sd-ddr50", 0x11000000 },
		{ "cdns,ctrl-hrs16-timing-delay-mmc-ddr52", 0x11000001 },
		{ "cdns,ctrl-hrs16-timing-delay-mmc-hs200", 0x00007777 },
		{ "cdns,ctrl-hrs16-timing-delay-mmc-hs400", 0x11000001 },
		{ "cdns,ctrl-hrs16-timing-delay-mmc-hs400es", 0x11000001 },
	},
	/* [4] ComboPHY: DQS timing */
	{
		{ "cdns,phy-dqs-timing-delay-default", 0x00780000 },
		{ "cdns,phy-dqs-timing-delay-mmc-hs", 0x00780000 },
		{ "cdns,phy-dqs-timing-delay-sd-hs", 0x00780001 },
		{ "cdns,phy-dqs-timing-delay-sd-sdr12", 0x00780000 },
		{ "cdns,phy-dqs-timing-delay-sd-sdr25", 0x00780001 },
		{ "cdns,phy-dqs-timing-delay-sd-sdr50", 0x00780004 },
		{ "cdns,phy-dqs-timing-delay-sd-sdr104", 0x00780004 },
		{ "cdns,phy-dqs-timing-delay-sd-ddr50", 0x00780004 },
		{ "cdns,phy-dqs-timing-delay-mmc-ddr52", 0x00780001 },
		{ "cdns,phy-dqs-timing-delay-mmc-hs200", 0x00780004 },
		{ "cdns,phy-dqs-timing-delay-mmc-hs400", 0x00680004 },
		{ "cdns,phy-dqs-timing-delay-mmc-hs400es", 0x00680004 },
	},
	/* [5] ComboPHY: PHY Gate Loopback Control */
	{
		{ "cdns,phy-gate-lpbk-ctrl-delay-default", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-mmc-hs", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-hs", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-sdr12", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-sdr25", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-sdr50", 0x80a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-sdr104", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-sd-ddr50", 0x80a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-mmc-ddr52", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-mmc-hs200", 0x81a40040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-mmc-hs400", 0x81fc0040 },
		{ "cdns,phy-gate-lpbk-ctrl-delay-mmc-hs400es", 0x81fc0040 },
	},
	/* [6] ComboPHY: PHY DLL Master Control */
	{
		{ "cdns,phy-dll-master-ctrl-default", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-mmc-hs", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-sd-hs", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-sd-sdr12", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-sd-sdr25", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-sd-sdr50", 0x00800004 },
		{ "cdns,phy-dll-master-ctrl-sd-sdr104", 0x00204d00 },
		{ "cdns,phy-dll-master-ctrl-sd-ddr50", 0x00800000 },
		{ "cdns,phy-dll-master-ctrl-mmc-ddr52", 0x00800000 },
		{ "cdns,phy-dll-master-ctrl-mmc-hs200", 0x00000004 },
		{ "cdns,phy-dll-master-ctrl-mmc-hs400", 0x00000004 },
		{ "cdns,phy-dll-master-ctrl-mmc-hs400es", 0x00000004 },
	},
	/* [7] ComboPHY: PHY DLL Slave Control */
	{
		{ "cdns,phy-dll-slave-ctrl-default", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-mmc-hs", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-sd-hs", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-sd-sdr12", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-sd-sdr25", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-sd-sdr50", 0x04000004 },
		{ "cdns,phy-dll-slave-ctrl-sd-sdr104", 0x04000004 },
		{ "cdns,phy-dll-slave-ctrl-sd-ddr50", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-mmc-ddr52", 0x00000000 },
		{ "cdns,phy-dll-slave-ctrl-mmc-hs200", 0x004d4d00 },
		{ "cdns,phy-dll-slave-ctrl-mmc-hs400", 0x004d4b40 },
		{ "cdns,phy-dll-slave-ctrl-mmc-hs400es", 0x004d4b40 },
	},
	/* [8] ComboPHY: DQ timing delay */
	{
		{ "cdns,phy-dq-timing-delay-default", 0x28000001 },
		{ "cdns,phy-dq-timing-delay-mmc-hs", 0x00000001 },
		{ "cdns,phy-dq-timing-delay-sd-hs", 0x10000001 },
		{ "cdns,phy-dq-timing-delay-sd-sdr12", 0x28000001 },
		{ "cdns,phy-dq-timing-delay-sd-sdr25", 0x10000001 },
		{ "cdns,phy-dq-timing-delay-sd-sdr50", 0x38000001 },
		{ "cdns,phy-dq-timing-delay-sd-sdr104", 0x38000001 },
		{ "cdns,phy-dq-timing-delay-sd-ddr50", 0x38000001 },
		{ "cdns,phy-dq-timing-delay-mmc-ddr52", 0x10000001 },
		{ "cdns,phy-dq-timing-delay-mmc-hs200", 0x10000001 },
		{ "cdns,phy-dq-timing-delay-mmc-hs400", 0x10000001 },
		{ "cdns,phy-dq-timing-delay-mmc-hs400es", 0x10000001 },
	}
};

static unsigned int sdhci_cdns6_read_phy_reg(struct sdhci_cdns_priv *priv,
					     const u32 address)
{
	writel(address, priv->hrs_addr + SDHCI_CDNS_HRS04);
	return readl(priv->hrs_addr + SDHCI_CDNS_HRS05);
}

static void sdhci_cdns6_write_phy_reg(struct sdhci_cdns_priv *priv,
				      const u32 address, const u32 value)
{
	writel(address, priv->hrs_addr + SDHCI_CDNS_HRS04);
	writel(value, priv->hrs_addr + SDHCI_CDNS_HRS05);
}

static int sdhci_cdns6_reset_phy_dll(struct mci_host *host, unsigned int reset)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS09;
	u32 tmp;
	int ret = 0;

	tmp = readl(reg);
	/* Switch On DLL Reset */
	tmp &= ~SDHCI_CDNS_HRS09_PHY_SW_RESET;

	if (!reset)
		/* Switch Off DLL Reset */
		tmp |= SDHCI_CDNS_HRS09_PHY_SW_RESET;

	writel(tmp, reg);

	/* After reset, wait until HRS09.PHY_INIT_COMPLETE is set to 1 within 3000us*/
	if (!reset) {
		ret = readl_poll_timeout(
			reg, tmp, (tmp & SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE),
			3000);
	}

	return ret;
}

int sdhci_cdns6_phy_adj(struct mci_host *host, unsigned char timing)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	struct device_node *np = host->hw_dev->of_node;
	struct sdhci_cdns6_ctrl_cfg timing_cfg[REG_CFG_MAX];
	u32 dt_value;
	void __iomem *reg;
	u32 tmp;
	int i;

	if (timing >= MAX_TIMING_MODES) {
		pr_err("Invalid timing mode: %d\n", timing);
		return -EINVAL;
	}

	/* Override values from device tree */
	for (i = 0; i < REG_CFG_MAX; i++) {
		const char *prop = reg_cfg[i][timing].property;

		if (!of_property_read_u32(np, prop, &dt_value)) {
			timing_cfg[i].value = dt_value;
			pr_debug("Overriding %s to 0x%08x from DT\n", prop,
				 dt_value);
		} else {
			timing_cfg[i].value = reg_cfg[i][timing].value;
		}
	}

	/* Switch On the DLL Reset */
	sdhci_cdns6_reset_phy_dll(host, true);

	sdhci_cdns6_write_phy_reg(priv, PHY_DQS_TIMING_REG_ADDR,
				  timing_cfg[REG_CFG_PHY_DQS].value);
	sdhci_cdns6_write_phy_reg(priv, PHY_GATE_LPBK_CTRL_REG_ADDR,
				  timing_cfg[REG_CFG_PHY_GATE_LPBK_CTRL].value);
	sdhci_cdns6_write_phy_reg(
		priv, PHY_DLL_MASTER_CTRL_REG_ADDR,
		timing_cfg[REG_CFG_PHY_DLL_MASTER_CTRL].value);
	sdhci_cdns6_write_phy_reg(priv, PHY_DLL_SLAVE_CTRL_REG_ADDR,
				  timing_cfg[REG_CFG_PHY_DLL_SLAVE_CTRL].value);

	/* Switch Off the DLL Reset */
	sdhci_cdns6_reset_phy_dll(host, false);

	/* Set PHY DQ TIMING control register */
	sdhci_cdns6_write_phy_reg(priv, PHY_DQ_TIMING_REG_ADDR,
				  timing_cfg[REG_CFG_PHY_DQ].value);

	/* Set HRS09 register */
	reg = priv->hrs_addr + SDHCI_CDNS_HRS09;
	tmp = readl(reg);
	tmp &= ~(SDHCI_CDNS_HRS09_EXTENDED_WR_MODE |
		 SDHCI_CDNS_HRS09_EXTENDED_RD_MODE |
		 SDHCI_CDNS_HRS09_RDDATA_EN | SDHCI_CDNS_HRS09_RDCMD_EN);
	tmp |= timing_cfg[REG_CFG_HRS09].value;
	writel(tmp, reg);

	/* Set HRS10 register */
	writel(timing_cfg[REG_CFG_HRS10].value,
	       priv->hrs_addr + SDHCI_CDNS_HRS10);

	/* Set HRS16 register */
	writel(timing_cfg[REG_CFG_HRS16].value,
	       priv->hrs_addr + SDHCI_CDNS_HRS16);

	/* Set HRS07 register */
	writel(timing_cfg[REG_CFG_HRS07].value,
	       priv->hrs_addr + SDHCI_CDNS_HRS07);

	return 0;
}

int sdhci_cdns6_set_tune_val(struct mci_host *host, unsigned int val)
{
	struct sdhci_cdns_priv *priv = sdhci_cdns_priv(host);
	u32 tmp, tuneval;

	/**
	 * Scale the tuning tap (0..39) to the 8-bit PHY DLL delay field (0..255),
	 * as required by the read_dqs_cmd_delay and read_dqs_delay fields.
	 * This ensures each tuning step maps linearly to the hardware delay range.
	 */
#define SDHCI_CDNS6_PHY_DLL_FIELD_SIZE 256

	tuneval = (val * SDHCI_CDNS6_PHY_DLL_FIELD_SIZE) /
		  SDHCI_CDNS_MAX_TUNING_LOOP;

	tmp = sdhci_cdns6_read_phy_reg(priv, PHY_DLL_SLAVE_CTRL_REG_ADDR);
	tmp &= ~(PHY_DLL_SLAVE_CTRL_REG_READ_DQS_CMD_DELAY |
		 PHY_DLL_SLAVE_CTRL_REG_READ_DQS_DELAY);
	tmp |= FIELD_PREP(PHY_DLL_SLAVE_CTRL_REG_READ_DQS_CMD_DELAY, tuneval) |
	       FIELD_PREP(PHY_DLL_SLAVE_CTRL_REG_READ_DQS_DELAY, tuneval);

	/* Switch On the DLL Reset */
	sdhci_cdns6_reset_phy_dll(host, true);

	sdhci_cdns6_write_phy_reg(priv, PHY_DLL_SLAVE_CTRL_REG_ADDR, tmp);

	/* Switch Off the DLL Reset */
	sdhci_cdns6_reset_phy_dll(host, false);

	return 0;
}

int sdhci_cdns6_phy_probe(struct mci_host *host)
{
	return sdhci_cdns6_phy_adj(host, MMC_TIMING_LEGACY);
}
