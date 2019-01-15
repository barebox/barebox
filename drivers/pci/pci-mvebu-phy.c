// SPDX-License-Identifier: GPL-2.0
/*
 * SoC specific PCIe PHY setup for Marvell MVEBU SoCs
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * based on Marvell BSP code (C) Marvell International Ltd.
 */

#include <common.h>
#include <of.h>
#include <of_address.h>

#include "pci-mvebu.h"

static u32 mvebu_pcie_phy_indirect(void __iomem *phybase, u8 lane,
				   u8 off, u16 val, bool is_read)
{
	u32 reg = (lane << 24) | (off << 16) | val;

	if (is_read)
		reg |= BIT(31);
	writel(reg, phybase);

	return (is_read) ? readl(phybase) & 0xffff : 0;
}

static inline u32 mvebu_pcie_phy_read(void __iomem *phybase, u8 lane,
				      u8 off)
{
	return mvebu_pcie_phy_indirect(phybase, lane, off, 0, true);
}

static inline void mvebu_pcie_phy_write(void __iomem *phybase, u8 lane,
					u8 off, u16 val)
{
	mvebu_pcie_phy_indirect(phybase, lane, off, val, false);
}

/* PCIe registers */
#define ARMADA_370_XP_PCIE_LINK_CAPS	0x6c
#define  MAX_LINK_WIDTH_MASK		MAX_LINK_WIDTH(0x3f)
#define  MAX_LINK_WIDTH(x)		((x) << 4)
#define  MAX_LINK_SPEED_MASK		0xf
#define  MAX_LINK_SPEED_5G0		0x2
#define  MAX_LINK_SPEED_2G5		0x1
#define ARMADA_370_XP_PHY_OFFSET	0x1b00
/* System Control registers */
#define ARMADA_370_XP_SOC_CTRL		0x04
#define  PCIE1_QUADX1_EN		BIT(8) /* Armada XP */
#define  PCIE0_QUADX1_EN		BIT(7) /* Armada XP */
#define  PCIE0_EN			BIT(0)
#define ARMADA_370_XP_SERDES03_SEL	0x70
#define ARMADA_370_XP_SERDES47_SEL	0x74
#define  SERDES(x, v)			((v) << ((x) * 0x4))
#define  SERDES_MASK(x)			SERDES((x), 0xf)

int armada_370_phy_setup(struct mvebu_pcie *pcie)
{
	struct device_node *np = of_find_compatible_node(NULL, NULL,
				 "marvell,armada-370-xp-system-controller");
	void __iomem *sysctrl = of_iomap(np, 0);
	void __iomem *phybase = pcie->base + ARMADA_370_XP_PHY_OFFSET;
	u32 reg;

	if (!sysctrl)
		return -ENODEV;

	/* Enable PEX */
	reg = readl(sysctrl + ARMADA_370_XP_SOC_CTRL);
	reg |= PCIE0_EN << pcie->port;
	writel(reg, sysctrl + ARMADA_370_XP_SOC_CTRL);

	/* Set SERDES selector */
	reg = readl(sysctrl + ARMADA_370_XP_SERDES03_SEL);
	reg &= ~SERDES_MASK(pcie->port);
	reg |= SERDES(pcie->port, 0x1);
	writel(reg, sysctrl + ARMADA_370_XP_SERDES03_SEL);

	/* BTS #232 - PCIe clock (undocumented) */
	writel(0x00000077, sysctrl + 0x2f0);

	/* Set x1 Link Capability */
	reg = readl(pcie->base + ARMADA_370_XP_PCIE_LINK_CAPS);
	reg &= ~(MAX_LINK_WIDTH_MASK | MAX_LINK_SPEED_MASK);
	reg |= MAX_LINK_WIDTH(0x1) | MAX_LINK_SPEED_5G0;
	writel(reg, pcie->base + ARMADA_370_XP_PCIE_LINK_CAPS);

	/* PEX pipe configuration */
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc1, 0x0025);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc3, 0x000f);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc8, 0x0005);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xd0, 0x0100);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xd1, 0x3014);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc5, 0x011f);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x80, 0x1000);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x81, 0x0011);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x0f, 0x2a21);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x45, 0x00df);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x4f, 0x6219);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x01, 0xfc60);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x46, 0x0000);

	reg = mvebu_pcie_phy_read(phybase, pcie->lane, 0x48) & ~0x4;
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x48, reg & 0xffff);

	mvebu_pcie_phy_write(phybase, pcie->lane, 0x02, 0x0040);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc1, 0x0024);

	mdelay(15);

	return 0;
}

/*
 * MV78230: 2 PCIe units Gen2.0, one unit 1x4 or 4x1, one unit 1x1
 * MV78260: 3 PCIe units Gen2.0, two units 1x4 or 4x1, one unit 1x1/1x4
 * MV78460: 4 PCIe units Gen2.0, two units 1x4 or 4x1, two units 1x1/1x4
 */
#define ARMADA_XP_COMM_PHY_REFCLK_ALIGN	0xf8
#define  REFCLK_ALIGN(x)	(0xf << ((x) * 0x4))
int armada_xp_phy_setup(struct mvebu_pcie *pcie)
{
	struct device_node *np = of_find_compatible_node(NULL, NULL,
				 "marvell,armada-370-xp-system-controller");
	void __iomem *sysctrl = of_iomap(np, 0);
	void __iomem *phybase = pcie->base + ARMADA_370_XP_PHY_OFFSET;
	u32 serdes_off = (pcie->port < 2) ? ARMADA_370_XP_SERDES03_SEL :
		ARMADA_370_XP_SERDES47_SEL;
	bool single_x4 = (pcie->lane_mask == 0xf);
	u32 reg, mask;

	if (!sysctrl)
		return -ENODEV;

	/* Prepare PEX */
	reg = readl(sysctrl + ARMADA_370_XP_SOC_CTRL);
	reg &= ~(PCIE0_EN << pcie->port);
	writel(reg, sysctrl + ARMADA_370_XP_SOC_CTRL);

	if (pcie->port < 2) {
		mask = PCIE0_QUADX1_EN << pcie->port;
		if (single_x4)
			reg &= ~mask;
		else
			reg |= mask;
	}
	reg |= PCIE0_EN << pcie->port;
	writel(reg, sysctrl + ARMADA_370_XP_SOC_CTRL);

	/* Set SERDES selector */
	reg = readl(sysctrl + serdes_off);
	for (mask = pcie->lane_mask; mask;) {
		u32 l = ffs(mask)-1;
		u32 off = 4 * (pcie->port % 2);
		reg &= ~SERDES_MASK(off + l);
		reg |= SERDES(off + l, 0x1);
		mask &= ~BIT(l);
	}
	reg &= ~SERDES_MASK(pcie->port % 2);
	reg |= SERDES(pcie->port % 2, 0x1);
	writel(reg, sysctrl + serdes_off);

	/* Reference Clock Alignment for 1x4 */
	reg = readl(sysctrl + ARMADA_XP_COMM_PHY_REFCLK_ALIGN);
	if (single_x4)
		reg |= REFCLK_ALIGN(pcie->port);
	else
		reg &= ~REFCLK_ALIGN(pcie->port);
	writel(reg, sysctrl + ARMADA_XP_COMM_PHY_REFCLK_ALIGN);

	/* Set x1/x4 Link Capability */
	reg = readl(pcie->base + ARMADA_370_XP_PCIE_LINK_CAPS);
	reg &= ~(MAX_LINK_WIDTH_MASK | MAX_LINK_SPEED_MASK);
	if (single_x4)
		reg |= MAX_LINK_WIDTH(0x4);
	else
		reg |= MAX_LINK_WIDTH(0x1);
	reg |= MAX_LINK_SPEED_5G0;
	writel(reg, pcie->base + ARMADA_370_XP_PCIE_LINK_CAPS);

	/* PEX pipe configuration */
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc1, 0x0025);
	if (single_x4) {
		mvebu_pcie_phy_write(phybase, pcie->lane, 0xc2, 0x0200);
		mvebu_pcie_phy_write(phybase, pcie->lane, 0xc3, 0x0001);
	} else {
		mvebu_pcie_phy_write(phybase, pcie->lane, 0xc2, 0x0000);
		mvebu_pcie_phy_write(phybase, pcie->lane, 0xc3, 0x000f);
	}
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc8, 0x0005);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x01, 0xfc60);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0x46, 0x0000);

	mvebu_pcie_phy_write(phybase, pcie->lane, 0x02, 0x0040);
	mvebu_pcie_phy_write(phybase, pcie->lane, 0xc1, 0x0024);
	if (single_x4)
		mvebu_pcie_phy_write(phybase, pcie->lane, 0x48, 0x1080);
	else
		mvebu_pcie_phy_write(phybase, pcie->lane, 0x48, 0x9080);

	mdelay(15);

	return 0;
}
