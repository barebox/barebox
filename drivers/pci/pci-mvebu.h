/*
 * PCIe include for Marvell MVEBU SoCs
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MVEBU_PCI_H
#define __MVEBU_PCI_H

#include <linux/pci.h>

struct mvebu_pcie {
	struct pci_controller pci;
	char *name;
	void __iomem *base;
	void __iomem *membase;
	struct resource mem;
	void __iomem *iobase;
	struct resource io;
	u32 port;
	u32 lane;
	u32 lane_mask;
	int devfn;
};

struct mvebu_pcie_ops {
	int (*phy_setup)(struct mvebu_pcie *pcie);
};

int armada_370_phy_setup(struct mvebu_pcie *pcie);
int armada_xp_phy_setup(struct mvebu_pcie *pcie);

#endif
