/*
 * Copyright (C) 2013
 *  Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *  Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <linux/clk.h>
#include <mach/common.h>

/*
 * Marvell MVEBU SoC id and revision can be read from any PCIe
 * controller port.
 */
u16 soc_devid;
EXPORT_SYMBOL(soc_devid);
u16 soc_revid;
EXPORT_SYMBOL(soc_revid);

static const struct of_device_id mvebu_pcie_of_ids[] = {
	{ .compatible = "marvell,armada-xp-pcie", },
	{ .compatible = "marvell,armada-370-pcie", },
	{ .compatible = "marvell,dove-pcie" },
	{ .compatible = "marvell,kirkwood-pcie" },
	{ },
};

#define PCIE_VEN_DEV_ID		0x000
#define PCIE_REV_ID		0x008
#define  REV_ID_MASK		0xff

static int mvebu_soc_id_init(void)
{
	struct device_node *np, *cnp;
	struct clk *clk;
	void __iomem *base;

	np = of_find_matching_node(NULL, mvebu_pcie_of_ids);
	if (!np)
		return -ENODEV;

	for_each_child_of_node(np, cnp) {
		base = of_iomap(cnp, 0);
		if (!base)
			continue;

		clk = of_clk_get(cnp, 0);
		if (IS_ERR(clk))
			continue;

		clk_enable(clk);
		soc_devid = readl(base + PCIE_VEN_DEV_ID) >> 16;
		soc_revid = readl(base + PCIE_REV_ID) & REV_ID_MASK;
		clk_disable(clk);
		break;
	}

	if (!soc_devid) {
		pr_err("Unable to read SoC id from PCIe ports\n");
		return -EINVAL;
	}

	pr_info("SoC: Marvell %04x rev %d\n", soc_devid, soc_revid);

	return 0;
}
postcore_initcall(mvebu_soc_id_init);

/*
 * Memory size is set up by BootROM and can be read from SoC's ram controller
 * registers. Fixup provided DTs to reflect accessible amount of directly
 * attached RAM. Removable RAM, e.g. SODIMM, should be added by a per-board
 * fixup.
 */
int mvebu_set_memory(u64 phys_base, u64 phys_size)
{
	struct device_node *np, *root;
	__be32 reg[4];
	int na, ns;

	root = of_get_root_node();
	if (!root)
		return -EINVAL;

	np = of_find_node_by_path("/memory");
	if (!np)
		np = of_create_node(root, "/memory");
	if (!np)
		return -EINVAL;

	na = of_n_addr_cells(np);
	ns = of_n_size_cells(np);

	if (na == 2) {
		reg[0] = cpu_to_be32(phys_base >> 32);
		reg[1] = cpu_to_be32(phys_base & 0xffffffff);
	} else {
		reg[0] = cpu_to_be32(phys_base & 0xffffffff);
	}

	if (ns == 2) {
		reg[2] = cpu_to_be32(phys_size >> 32);
		reg[3] = cpu_to_be32(phys_size & 0xffffffff);
	} else {
		reg[1] = cpu_to_be32(phys_size & 0xffffffff);
	}

	if (of_set_property(np, "device_type", "memory", sizeof("memory"), 1) ||
	    of_set_property(np, "reg", reg, sizeof(u32) * (na + ns), 1))
		pr_err("Unable to fixup memory node\n");

	return 0;
}

static __noreturn void (*mvebu_reset_cpu)(unsigned long addr);

void __noreturn reset_cpu(unsigned long addr)
{
	mvebu_reset_cpu(addr);
}
EXPORT_SYMBOL(reset_cpu);

void mvebu_set_reset(void __noreturn (*reset)(unsigned long addr))
{
	mvebu_reset_cpu = reset;
}
