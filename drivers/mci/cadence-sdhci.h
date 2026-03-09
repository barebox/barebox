/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 */

#ifndef SDHCI_CADENCE_H_
#define SDHCI_CADENCE_H_

#include <common.h>
#include <clock.h>
#include <linux/clk.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/iopoll.h>
#include <linux/reset.h>

#include "sdhci.h"

/* HRS - Host Register Set (specific to Cadence) */
#define SDHCI_CDNS_HRS04	0x10		/* PHY access port */
#define SDHCI_CDNS_HRS05        0x14		/* PHY data access port */

/*
 * The tuned val register is 6 bit-wide, but not the whole of the range is
 * available. The range 0-42 seems to be available (then 43 wraps around to 0)
 * but I am not quite sure if it is official.  Use only 0 to 39 for safety.
 */
#define SDHCI_CDNS_MAX_TUNING_LOOP	40

struct sdhci_cdns_drv_data {
	const struct mci_ops *ops;
	unsigned int quirks;
	unsigned int quirks2;
};

/**
 * struct sdhci_cdns4_phy_param - PHY parameter/address pair
 * @addr: PHY register address.
 * @data: Value to write to the PHY register.
 *
 * Used for passing a list of PHY configuration parameters to the
 * Cadence SDHCI V4 controller.
 */
struct sdhci_cdns4_phy_param {
	u8 addr;
	u8 data;
};

/**
 * struct sdhci_cdns_priv - Cadence SDHCI private controller data
 * @hrs_addr: Base address of Cadence Host Register Set (HRS) registers.
 * @ctl_addr: Base address for write control registers.
 *            Used only for "amd,pensando-elba-sd4hc" compatible controllers
 *            to enable byte-lane writes.
 * @wrlock: Spinlock for protecting register writes (Elba only).
 * @enhanced_strobe: Flag indicating if Enhanced Strobe (HS400ES) is enabled.
 * @priv_writel: Optional SoC-specific write function for register access.
 *               Used for Elba to ensure correct byte-lane enable.
 * @rst_hw: Hardware reset control for the controller.
 * @ciu_clk: Card Interface Unit (CIU) clock handle.
 *           Used only for V6 (SDHCI spec >= 4.20) controllers.
 * @nr_phy_params: Number of PHY parameter entries parsed from DT (V4 only).
 * @phy_params: Array of PHY parameter/address pairs for PHY initialization (V4 only).
 */
struct sdhci_cdns_priv {
	struct mci_host mci;
	struct sdhci sdhci;
	struct reset_control *rst;
	struct reset_control *softphy_rst;
	struct reset_control *sdmmc_ocp_rst;
	void __iomem *hrs_addr;
	void __iomem *ctl_addr; /* write control */
	bool enhanced_strobe;
	struct clk *biu_clk; /* Card Interface Unit clock */
	struct clk *ciu_clk; /* Card Interface Unit clock */
	unsigned int nr_phy_params;
	struct sdhci_cdns4_phy_param phy_params[];
};

/*
 * sdhci_cdns_priv - Helper to retrieve Cadence private data from sdhci_host
 * @host: Pointer to struct sdhci_host.
 *
 * Returns: Pointer to struct sdhci_cdns_priv.
 */
static inline void *sdhci_cdns_priv(struct mci_host *host)
{
	return host->hw_dev->priv;
}

/**
 * sdhci_cdns6_phy_probe - Initialize the Cadence PHY using device tree.
 * @host: Pointer to struct sdhci_host.
 *
 * Returns 0 on success or a negative error code.
 */
int sdhci_cdns6_phy_probe(struct mci_host *host);

/**
 * sdhci_cdns6_phy_adj - Program PHY registers for a specific timing mode.
 * @host: Pointer to struct sdhci_host.
 * @timing: MMC timing mode (MMC_TIMING_*).
 *
 * Returns 0 on success or a negative error code.
 */
int sdhci_cdns6_phy_adj(struct mci_host *host, unsigned char timing);

/**
 * sdhci_cdns6_set_tune_val - Set the PHY tuning value.
 * @host: Pointer to struct sdhci_host.
 * @val: Tuning value to program.
 *
 * Returns 0 on success or a negative error code.
 */
int sdhci_cdns6_set_tune_val(struct mci_host *host, unsigned int val);

#endif // SDHCI_CADENCE_H_
