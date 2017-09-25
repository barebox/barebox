/*
 * PCIe host controller driver for Freescale i.MX6 SoCs
 *
 * Copyright (C) 2013 Kosagi
 *		http://www.kosagi.com
 *
 * Author: Sean Cross <xobs@kosagi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <clock.h>
#include <abort.h>
#include <malloc.h>
#include <io.h>
#include <init.h>
#include <gpio.h>
#include <asm/mmu.h>
#include <of_gpio.h>
#include <of_device.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <of_address.h>
#include <of_pci.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/sizes.h>
#include <mfd/imx6q-iomuxc-gpr.h>

#include <mach/imx6-regs.h>

#include "pcie-designware.h"

#define to_imx6_pcie(x)	container_of(x, struct imx6_pcie, pp)

enum imx6_pcie_variants {
	IMX6Q,
	IMX6QP,
};

struct imx6_pcie {
	int			reset_gpio;
	struct clk		*pcie_bus;
	struct clk		*pcie_phy;
	struct clk		*pcie;
	struct pcie_port	pp;
	void __iomem		*iomuxc_gpr;
	enum imx6_pcie_variants variant;
	void __iomem		*mem_base;
	u32                     tx_deemph_gen1;
	u32                     tx_deemph_gen2_3p5db;
	u32                     tx_deemph_gen2_6db;
	u32                     tx_swing_full;
	u32                     tx_swing_low;
};

/* PCIe Root Complex registers (memory-mapped) */
#define PCIE_RC_LCR				0x7c
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1	0x1
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2	0x2
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK	0xf

#define PCIE_RC_LCSR				0x80

/* PCIe Port Logic registers (memory-mapped) */
#define PL_OFFSET 0x700
#define PCIE_PL_PFLR (PL_OFFSET + 0x08)
#define PCIE_PL_PFLR_LINK_STATE_MASK		(0x3f << 16)
#define PCIE_PL_PFLR_FORCE_LINK			(1 << 15)
#define PCIE_PHY_DEBUG_R0 (PL_OFFSET + 0x28)
#define PCIE_PHY_DEBUG_R1 (PL_OFFSET + 0x2c)
#define PCIE_PHY_DEBUG_R1_XMLH_LINK_IN_TRAINING	(1 << 29)
#define PCIE_PHY_DEBUG_R1_XMLH_LINK_UP		(1 << 4)

#define PCIE_PHY_CTRL (PL_OFFSET + 0x114)
#define PCIE_PHY_CTRL_DATA_LOC 0
#define PCIE_PHY_CTRL_CAP_ADR_LOC 16
#define PCIE_PHY_CTRL_CAP_DAT_LOC 17
#define PCIE_PHY_CTRL_WR_LOC 18
#define PCIE_PHY_CTRL_RD_LOC 19

#define PCIE_PHY_STAT (PL_OFFSET + 0x110)
#define PCIE_PHY_STAT_ACK_LOC 16

#define PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define PORT_LOGIC_SPEED_CHANGE		(0x1 << 17)

/* PHY registers (not memory-mapped) */
#define PCIE_PHY_RX_ASIC_OUT 0x100D

#define PHY_RX_OVRD_IN_LO 0x1005
#define PHY_RX_OVRD_IN_LO_RX_DATA_EN (1 << 5)
#define PHY_RX_OVRD_IN_LO_RX_PLL_EN (1 << 3)

static int pcie_phy_poll_ack(void __iomem *dbi_base, int exp_val)
{
	u32 val;
	u32 max_iterations = 10;
	u32 wait_counter = 0;

	do {
		val = readl(dbi_base + PCIE_PHY_STAT);
		val = (val >> PCIE_PHY_STAT_ACK_LOC) & 0x1;
		wait_counter++;

		if (val == exp_val)
			return 0;

		udelay(1);
	} while (wait_counter < max_iterations);

	return -ETIMEDOUT;
}

static int pcie_phy_wait_ack(void __iomem *dbi_base, int addr)
{
	u32 val;
	int ret;

	val = addr << PCIE_PHY_CTRL_DATA_LOC;
	writel(val, dbi_base + PCIE_PHY_CTRL);

	val |= (0x1 << PCIE_PHY_CTRL_CAP_ADR_LOC);
	writel(val, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	val = addr << PCIE_PHY_CTRL_DATA_LOC;
	writel(val, dbi_base + PCIE_PHY_CTRL);

	return pcie_phy_poll_ack(dbi_base, 0);
}

/* Read from the 16-bit PCIe PHY control registers (not memory-mapped) */
static int pcie_phy_read(void __iomem *dbi_base, int addr , int *data)
{
	u32 val, phy_ctl;
	int ret;

	ret = pcie_phy_wait_ack(dbi_base, addr);
	if (ret)
		return ret;

	/* assert Read signal */
	phy_ctl = 0x1 << PCIE_PHY_CTRL_RD_LOC;
	writel(phy_ctl, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	val = readl(dbi_base + PCIE_PHY_STAT);
	*data = val & 0xffff;

	/* deassert Read signal */
	writel(0x00, dbi_base + PCIE_PHY_CTRL);

	return pcie_phy_poll_ack(dbi_base, 0);
}

static int pcie_phy_write(void __iomem *dbi_base, int addr, int data)
{
	u32 var;
	int ret;

	/* write addr */
	/* cap addr */
	ret = pcie_phy_wait_ack(dbi_base, addr);
	if (ret)
		return ret;

	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* capture data */
	var |= (0x1 << PCIE_PHY_CTRL_CAP_DAT_LOC);
	writel(var, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	/* deassert cap data */
	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack de-assertion */
	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	/* assert wr signal */
	var = 0x1 << PCIE_PHY_CTRL_WR_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack */
	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	/* deassert wr signal */
	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack de-assertion */
	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	writel(0x0, dbi_base + PCIE_PHY_CTRL);

	return 0;
}

static void imx6_pcie_reset_phy(struct pcie_port *pp)
{
	uint32_t temp;

	pcie_phy_read(pp->dbi_base, PHY_RX_OVRD_IN_LO, &temp);
	temp |= (PHY_RX_OVRD_IN_LO_RX_DATA_EN |
		 PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write(pp->dbi_base, PHY_RX_OVRD_IN_LO, temp);

	udelay(2000);

	pcie_phy_read(pp->dbi_base, PHY_RX_OVRD_IN_LO, &temp);
	temp &= ~(PHY_RX_OVRD_IN_LO_RX_DATA_EN |
		  PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write(pp->dbi_base, PHY_RX_OVRD_IN_LO, temp);
}

static int imx6_pcie_assert_core_reset(struct pcie_port *pp)
{
	struct imx6_pcie *imx6_pcie = to_imx6_pcie(pp);
	u32 val, gpr1, gpr12;

	switch (imx6_pcie->variant) {
	case IMX6QP:
		gpr1 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
		gpr1 |= IMX6Q_GPR1_PCIE_SW_RST;
		writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
		break;
	case IMX6Q:
		/*
		 * If the bootloader already enabled the link we need some special
		 * handling to get the core back into a state where it is safe to
		 * touch it for configuration.  As there is no dedicated reset signal
		 * wired up for MX6QDL, we need to manually force LTSSM into "detect"
		 * state before completely disabling LTSSM, which is a prerequisite
		 * for core configuration.
		 *
		 * If both LTSSM_ENABLE and REF_SSP_ENABLE are active we have a strong
		 * indication that the bootloader activated the link.
		 */
		gpr1 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
		gpr12 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);

		if ((gpr1 & IMX6Q_GPR1_PCIE_REF_CLK_EN) &&
		    (gpr12 & IMX6Q_GPR12_PCIE_CTL_2)) {
			val = readl(pp->dbi_base + PCIE_PL_PFLR);
			val &= ~PCIE_PL_PFLR_LINK_STATE_MASK;
			val |= PCIE_PL_PFLR_FORCE_LINK;

			data_abort_mask();
			writel(val, pp->dbi_base + PCIE_PL_PFLR);
			data_abort_unmask();

			gpr12 &= ~IMX6Q_GPR12_PCIE_CTL_2;
			writel(gpr12, imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);
		}

		gpr1 |= IMX6Q_GPR1_PCIE_TEST_PD;
		writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);

		gpr1 &= ~IMX6Q_GPR1_PCIE_REF_CLK_EN;
		writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
		break;
	}

	return 0;
}

static int imx6_pcie_deassert_core_reset(struct pcie_port *pp)
{
	struct imx6_pcie *imx6_pcie = to_imx6_pcie(pp);
	int ret;
	u32 gpr1;

	ret = clk_enable(imx6_pcie->pcie_phy);
	if (ret)
		goto err_pcie_phy;

	ret = clk_enable(imx6_pcie->pcie_bus);
	if (ret)
		goto err_pcie_bus;

	ret = clk_enable(imx6_pcie->pcie);
	if (ret)
		goto err_pcie;

	/* power up core phy and enable ref clock */
	gpr1 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
	gpr1 &= ~IMX6Q_GPR1_PCIE_TEST_PD;
	writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
	/*
	 * the async reset input need ref clock to sync internally,
	 * when the ref clock comes after reset, internal synced
	 * reset time is too short, cannot meet the requirement.
	 * add one ~10us delay here.
	 */
	udelay(10);
	gpr1 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
	gpr1 |= IMX6Q_GPR1_PCIE_REF_CLK_EN;
	writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);

	/* allow the clocks to stabilize */
	udelay(200);

	/* Some boards don't have PCIe reset GPIO. */
	if (gpio_is_valid(imx6_pcie->reset_gpio)) {
		gpio_set_value(imx6_pcie->reset_gpio, 0);
		mdelay(100);
		gpio_set_value(imx6_pcie->reset_gpio, 1);
	}

	/*
	 * Release the PCIe PHY reset here
	 */
	switch (imx6_pcie->variant) {
	case IMX6QP:
		gpr1 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);
		gpr1 &= ~IMX6Q_GPR1_PCIE_SW_RST;
		writel(gpr1, imx6_pcie->iomuxc_gpr + IOMUXC_GPR1);

		udelay(200);
		break;
	case IMX6Q:		/* Nothing to do */
		break;
	}

	return 0;

err_pcie:
	clk_disable(imx6_pcie->pcie_bus);
err_pcie_bus:
	clk_disable(imx6_pcie->pcie_phy);
err_pcie_phy:
	return ret;

}

static void imx6_pcie_init_phy(struct pcie_port *pp)
{
	struct imx6_pcie *imx6_pcie = to_imx6_pcie(pp);
	u32 gpr12, gpr8;

	gpr12 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);
	gpr12 &= ~IMX6Q_GPR12_PCIE_CTL_2;
	writel(gpr12, imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);

	/* configure constant input signal to the pcie ctrl and phy */
	gpr12 &= ~IMX6Q_GPR12_DEVICE_TYPE;
	gpr12 |= PCI_EXP_TYPE_ROOT_PORT << 12;
	writel(gpr12, imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);

	gpr12 &= ~IMX6Q_GPR12_LOS_LEVEL;
	gpr12 |= 9 << 4;
	writel(gpr12, imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);

	gpr8 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);
	gpr8 &= ~IMX6Q_GPR8_TX_DEEMPH_GEN1;
	gpr8 |= imx6_pcie->tx_deemph_gen1 << 0;
	writel(gpr8, imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);

	gpr8 &= ~IMX6Q_GPR8_TX_DEEMPH_GEN2_3P5DB;
	gpr8 |= imx6_pcie->tx_deemph_gen2_3p5db << 6;
	writel(gpr8, imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);

	gpr8 &= ~IMX6Q_GPR8_TX_DEEMPH_GEN2_6DB;
	gpr8 |= imx6_pcie->tx_deemph_gen2_6db << 12;
	writel(gpr8, imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);

	gpr8 &= ~IMX6Q_GPR8_TX_SWING_FULL;
	gpr8 |= imx6_pcie->tx_swing_full << 18;
	writel(gpr8, imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);

	gpr8 &= ~IMX6Q_GPR8_TX_SWING_LOW;
	gpr8 |= imx6_pcie->tx_swing_low << 25;
	writel(gpr8, imx6_pcie->iomuxc_gpr + IOMUXC_GPR8);
}

static int imx6_pcie_wait_for_link(struct pcie_port *pp)
{
	uint64_t start = get_time_ns();

	while (1) {
		if (dw_pcie_link_up(pp))
			return 0;

		if (!is_timeout(start, SECOND))
			continue;

		return -EINVAL;
	}
}

static int imx6_pcie_wait_for_speed_change(struct pcie_port *pp)
{
	uint32_t tmp;
	uint64_t start = get_time_ns();

	while (!is_timeout(start, SECOND)) {
		tmp = readl(pp->dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
		/* Test if the speed change finished. */
		if (!(tmp & PORT_LOGIC_SPEED_CHANGE))
			return 0;
	}

	dev_err(pp->dev, "Speed change timeout\n");
	return -EINVAL;
}


static int imx6_pcie_establish_link(struct pcie_port *pp)
{
	struct imx6_pcie *imx6_pcie = to_imx6_pcie(pp);
	uint32_t tmp;
	int ret;
	u32 gpr12;

	/*
	 * Force Gen1 operation when starting the link.  In case the link is
	 * started in Gen2 mode, there is a possibility the devices on the
	 * bus will not be detected at all.  This happens with PCIe switches.
	 */
	tmp = readl(pp->dbi_base + PCIE_RC_LCR);
	tmp &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK;
	tmp |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1;
	writel(tmp, pp->dbi_base + PCIE_RC_LCR);

	/* Start LTSSM. */
	gpr12 = readl(imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);
	gpr12 |= IMX6Q_GPR12_PCIE_CTL_2;
	writel(gpr12, imx6_pcie->iomuxc_gpr + IOMUXC_GPR12);

	ret = imx6_pcie_wait_for_link(pp);
	if (ret) {
		dev_info(pp->dev, "Link never came up\n");
		goto err_reset_phy;
	}

	/* Allow Gen2 mode after the link is up. */
	tmp = readl(pp->dbi_base + PCIE_RC_LCR);
	tmp &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK;
	tmp |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2;
	writel(tmp, pp->dbi_base + PCIE_RC_LCR);

	/*
	 * Start Directed Speed Change so the best possible speed both link
	 * partners support can be negotiated.
	 */
	tmp = readl(pp->dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
	tmp |= PORT_LOGIC_SPEED_CHANGE;
	writel(tmp, pp->dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);

	ret = imx6_pcie_wait_for_speed_change(pp);
	if (ret) {
		dev_err(pp->dev, "Failed to bring link up!\n");
		goto err_reset_phy;
	}

	/* Make sure link training is finished as well! */
	ret = imx6_pcie_wait_for_link(pp);
	if (ret) {
		dev_err(pp->dev, "Failed to bring link up!\n");
		goto err_reset_phy;
	}

	tmp = readl(pp->dbi_base + PCIE_RC_LCSR);
	dev_dbg(pp->dev, "Link up, Gen=%i\n", (tmp >> 16) & 0xf);

	return 0;

err_reset_phy:
       dev_dbg(pp->dev, "PHY DEBUG_R0=0x%08x DEBUG_R1=0x%08x\n",
               readl(pp->dbi_base + PCIE_PHY_DEBUG_R0),
               readl(pp->dbi_base + PCIE_PHY_DEBUG_R1));
       imx6_pcie_reset_phy(pp);

       return ret;
}

static void imx6_pcie_host_init(struct pcie_port *pp)
{
	imx6_pcie_assert_core_reset(pp);

	imx6_pcie_init_phy(pp);

	imx6_pcie_deassert_core_reset(pp);

	dw_pcie_setup_rc(pp);

	imx6_pcie_establish_link(pp);
}

static int imx6_pcie_link_up(struct pcie_port *pp)
{
	u32 rc;
	int count = 5;

	/*
	 * Test if the PHY reports that the link is up and also that the LTSSM
	 * training finished. There are three possible states of the link when
	 * this code is called:
	 * 1) The link is DOWN (unlikely)
	 *     The link didn't come up yet for some reason. This usually means
	 *     we have a real problem somewhere. Reset the PHY and exit. This
	 *     state calls for inspection of the DEBUG registers.
	 * 2) The link is UP, but still in LTSSM training
	 *     Wait for the training to finish, which should take a very short
	 *     time. If the training does not finish, we have a problem and we
	 *     need to inspect the DEBUG registers. If the training does finish,
	 *     the link is up and operating correctly.
	 * 3) The link is UP and no longer in LTSSM training
	 *     The link is up and operating correctly.
	 */
	while (1) {
		rc = readl(pp->dbi_base + PCIE_PHY_DEBUG_R1);
		if (!(rc & PCIE_PHY_DEBUG_R1_XMLH_LINK_UP))
			break;
		if (!(rc & PCIE_PHY_DEBUG_R1_XMLH_LINK_IN_TRAINING))
			return 1;
		if (!count--)
			break;
		dev_dbg(pp->dev, "Link is up, but still in training\n");
		/*
		 * Wait a little bit, then re-check if the link finished
		 * the training.
		 */
		udelay(1000);
	}

	return 0;
}

static struct pcie_host_ops imx6_pcie_host_ops = {
	.link_up = imx6_pcie_link_up,
	.host_init = imx6_pcie_host_init,
};

static int __init imx6_add_pcie_port(struct pcie_port *pp,
			struct device_d *dev)
{
	int ret;

	pp->root_bus_nr = -1;
	pp->ops = &imx6_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static int __init imx6_pcie_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imx6_pcie *imx6_pcie;
	struct pcie_port *pp;
	struct device_node *np = dev->device_node;
	int ret;

	imx6_pcie = xzalloc(sizeof(*imx6_pcie));

	pp = &imx6_pcie->pp;
	pp->dev = dev;

	imx6_pcie->variant =
		(enum imx6_pcie_variants)of_device_get_match_data(dev);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	pp->dbi_base = IOMEM(iores->start);

	/* Fetch GPIOs */
	imx6_pcie->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (gpio_is_valid(imx6_pcie->reset_gpio)) {
		ret = gpio_request_one(imx6_pcie->reset_gpio,
					    GPIOF_OUT_INIT_LOW, "PCIe reset");
		if (ret) {
			dev_err(dev, "unable to get reset gpio\n");
			return ret;
		}
	}

	/* Fetch clocks */
	imx6_pcie->pcie_phy = clk_get(dev, "pcie_phy");
	if (IS_ERR(imx6_pcie->pcie_phy)) {
		dev_err(dev, "pcie_phy clock source missing or invalid\n");
		return PTR_ERR(imx6_pcie->pcie_phy);
	}

	imx6_pcie->pcie_bus = clk_get(dev, "pcie_bus");
	if (IS_ERR(imx6_pcie->pcie_bus)) {
		dev_err(dev, "pcie_bus clock source missing or invalid\n");
		return PTR_ERR(imx6_pcie->pcie_bus);
	}

	imx6_pcie->pcie = clk_get(dev, "pcie");
	if (IS_ERR(imx6_pcie->pcie)) {
		dev_err(dev, "pcie clock source missing or invalid\n");
		return PTR_ERR(imx6_pcie->pcie);
	}

	/* Grab GPR config register range */
	imx6_pcie->iomuxc_gpr = IOMEM(MX6_IOMUXC_BASE_ADDR);

	/* Grab PCIe PHY Tx Settings */
	if (of_property_read_u32(np, "fsl,tx-deemph-gen1",
				 &imx6_pcie->tx_deemph_gen1))
		imx6_pcie->tx_deemph_gen1 = 0;

	if (of_property_read_u32(np, "fsl,tx-deemph-gen2-3p5db",
				 &imx6_pcie->tx_deemph_gen2_3p5db))
		imx6_pcie->tx_deemph_gen2_3p5db = 0;

	if (of_property_read_u32(np, "fsl,tx-deemph-gen2-6db",
				 &imx6_pcie->tx_deemph_gen2_6db))
		imx6_pcie->tx_deemph_gen2_6db = 20;

	if (of_property_read_u32(np, "fsl,tx-swing-full",
				 &imx6_pcie->tx_swing_full))
		imx6_pcie->tx_swing_full = 127;

	if (of_property_read_u32(np, "fsl,tx-swing-low",
				 &imx6_pcie->tx_swing_low))
		imx6_pcie->tx_swing_low = 127;

	ret = imx6_add_pcie_port(pp, dev);
	if (ret < 0)
		return ret;

	dev->priv = imx6_pcie;

	return 0;
}

static void imx6_pcie_remove(struct device_d *dev)
{
	struct imx6_pcie *imx6_pcie = dev->priv;

	imx6_pcie_assert_core_reset(&imx6_pcie->pp);
}

static struct of_device_id imx6_pcie_of_match[] = {
	{ .compatible = "fsl,imx6q-pcie",  .data = (void *)IMX6Q,  },
	{ .compatible = "fsl,imx6qp-pcie", .data = (void *)IMX6QP, },
	{},
};

static struct driver_d imx6_pcie_driver = {
	.name = "imx6-pcie",
	.of_compatible = DRV_OF_COMPAT(imx6_pcie_of_match),
	.probe = imx6_pcie_probe,
	.remove = imx6_pcie_remove,
};
device_platform_driver(imx6_pcie_driver);

MODULE_AUTHOR("Sean Cross <xobs@kosagi.com>");
MODULE_DESCRIPTION("Freescale i.MX6 PCIe host controller driver");
MODULE_LICENSE("GPL v2");
