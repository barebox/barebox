// SPDX-License-Identifier: GPL-2.0
/*
 * Synopsys DesignWare PCIe host controller driver
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jingoo Han <jg1.han@samsung.com>
 */


#include <common.h>
#include <clock.h>
#include <malloc.h>
#include <io.h>
#include <init.h>
#include <asm/mmu.h>

#include <linux/clk.h>
#include <linux/kernel.h>
#include <of_address.h>
#include <of_pci.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/sizes.h>

#include "pcie-designware.h"

int dw_pcie_read(void __iomem *addr, int size, u32 *val)
{
	if ((uintptr_t)addr & (size - 1)) {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	if (size == 4) {
		*val = readl(addr);
	} else if (size == 2) {
		*val = readw(addr);
	} else if (size == 1) {
		*val = readb(addr);
	} else {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	return PCIBIOS_SUCCESSFUL;
}

int dw_pcie_write(void __iomem *addr, int size, u32 val)
{
	if ((uintptr_t)addr & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (size == 4)
		writel(val, addr);
	else if (size == 2)
		writew(val, addr);
	else if (size == 1)
		writeb(val, addr);
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

u32 __dw_pcie_readl_dbi(struct dw_pcie *pci, void __iomem *base, u32 reg,
			size_t size)
{
	int ret;
	u32 val;

	if (pci->ops->readl_dbi)
		return pci->ops->readl_dbi(pci, base, reg, size);

	ret = dw_pcie_read(base + reg, size, &val);
	if (ret)
		dev_err(pci->dev, "Read DBI address failed\n");

	return val;
}

void __dw_pcie_writel_dbi(struct dw_pcie *pci, void __iomem *base, u32 reg,
			  size_t size, u32 val)
{
	int ret;

	if (pci->ops->writel_dbi) {
		pci->ops->writel_dbi(pci, base, reg, size, val);
		return;
	}

	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "Write DBI address failed\n");
}

static u32 dw_pcie_readl_ob_unroll(struct dw_pcie *pci, u32 index, u32 reg)
{
	u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

	return dw_pcie_readl_dbi(pci, offset + reg);
}

static void dw_pcie_writel_ob_unroll(struct dw_pcie *pci, u32 index,
				     u32 reg, u32 val)
{
	u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

	dw_pcie_writel_dbi(pci, offset + reg, val);
}

static void dw_pcie_prog_outbound_atu_unroll(struct dw_pcie *pci, int index,
					     int type, u64 cpu_addr,
					     u64 pci_addr, u32 size)
{
	u32 retries, val;

	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LOWER_BASE,
				 lower_32_bits(cpu_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_UPPER_BASE,
				 upper_32_bits(cpu_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LIMIT,
				 lower_32_bits(cpu_addr + size - 1));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LOWER_TARGET,
				 lower_32_bits(pci_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_UPPER_TARGET,
				 upper_32_bits(pci_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL1,
				 type);
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL2,
				 PCIE_ATU_ENABLE);

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_ob_unroll(pci, index,
					      PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE)
			return;

		udelay(LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "Outbound iATU is not being enabled\n");
}

void dw_pcie_prog_outbound_atu(struct dw_pcie *pci, int index,
			       int type, u64 cpu_addr, u64 pci_addr,
			       u32 size)
{
	u32 retries, val;

	if (pci->iatu_unroll_enabled) {
		dw_pcie_prog_outbound_atu_unroll(pci, index, type, cpu_addr,
						 pci_addr, size);
		return;
	}

	dw_pcie_writel_dbi(pci, PCIE_ATU_VIEWPORT,
			   PCIE_ATU_REGION_OUTBOUND | index);
	dw_pcie_writel_dbi(pci, PCIE_ATU_LOWER_BASE,
			   lower_32_bits(cpu_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_UPPER_BASE,
			   upper_32_bits(cpu_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_LIMIT,
			   lower_32_bits(cpu_addr + size - 1));
	dw_pcie_writel_dbi(pci, PCIE_ATU_LOWER_TARGET,
			   lower_32_bits(pci_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_UPPER_TARGET,
			   upper_32_bits(pci_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR1, type);
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR2, PCIE_ATU_ENABLE);

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_dbi(pci, PCIE_ATU_CR2);
		if (val & PCIE_ATU_ENABLE)
			return;

		udelay(LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "Outbound iATU is not being enabled\n");
}

int dw_pcie_wait_for_link(struct dw_pcie *pci)
{
	int retries;

	/* Check if the link is up or not */
	for (retries = 0; retries < LINK_WAIT_MAX_RETRIES; retries++) {
		if (dw_pcie_link_up(pci)) {
			dev_info(pci->dev, "Link up\n");
			return 0;
		}
		udelay(LINK_WAIT_USLEEP_MAX);
	}

	dev_err(pci->dev, "Phy link never came up\n");

	return -ETIMEDOUT;
}

int dw_pcie_link_up(struct dw_pcie *pci)
{
	u32 val;

	if (pci->ops->link_up)
		return pci->ops->link_up(pci);

	val = readl(pci->dbi_base + PCIE_PHY_DEBUG_R1);
	return ((val & PCIE_PHY_DEBUG_R1_LINK_UP) &&
		!(val & PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING));
}

void dw_pcie_setup(struct dw_pcie *pci)
{
	int ret;
	u32 val;
	u32 lanes;
	struct device_d *dev = pci->dev;
	struct device_node *np = dev->device_node;

	ret = of_property_read_u32(np, "num-lanes", &lanes);
	if (ret)
		lanes = 0;

	/* Set the number of lanes */
	val = dw_pcie_readl_dbi(pci, PCIE_PORT_LINK_CONTROL);
	val &= ~PORT_LINK_MODE_MASK;
	switch (lanes) {
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
       default:
               dev_err(pci->dev, "num-lanes %u: invalid value\n", lanes);
               return;
	}
	dw_pcie_writel_dbi(pci, PCIE_PORT_LINK_CONTROL, val);

	/* Set link width speed control register */
	val = dw_pcie_readl_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (lanes) {
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	}
	dw_pcie_writel_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL, val);
}

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_DESCRIPTION("Designware PCIe host controller driver");
MODULE_LICENSE("GPL v2");
