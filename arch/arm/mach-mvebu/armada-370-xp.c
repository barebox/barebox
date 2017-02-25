/*
 * Copyright
 * (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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
#include <restart.h>
#include <of.h>
#include <of_address.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <mach/armada-370-xp-regs.h>
#include <mach/socid.h>

static const struct of_device_id armada_370_xp_pcie_of_ids[] = {
	{ .compatible = "marvell,armada-xp-pcie", },
	{ .compatible = "marvell,armada-370-pcie", },
	{ },
};

/*
 * Marvell Armada XP MV78230-A0 incorrectly identifies itself as
 * MV78460. Check for DEVID_MV78460 but if there are only 2 CPUs
 * present in Coherency Fabric, fixup PCIe PRODUCT_ID.
 */
static int armada_xp_soc_id_fixup(void)
{
	struct device_node *np, *cnp;
	void __iomem *base;
	u32 reg, ctrl;
	u32 socid, numcpus;

	socid = readl(ARMADA_370_XP_CPU_SOC_ID) & CPU_SOC_ID_DEVICE_MASK;
	numcpus = 1 + (readl(ARMADA_370_XP_FABRIC_CONF) & FABRIC_NUM_CPUS_MASK);

	if (socid != DEVID_MV78460 || numcpus != 2)
		/* not affected */
		return 0;

	np = of_find_matching_node(NULL, armada_370_xp_pcie_of_ids);
	if (!np)
		return -ENODEV;

	/* Enable all individual x1 ports */
	ctrl = readl(ARMADA_370_XP_SOC_CTRL);
	writel(ctrl | PCIE0_EN | PCIE1_EN | PCIE0_QUADX1_EN, ARMADA_370_XP_SOC_CTRL);

	for_each_child_of_node(np, cnp) {
		base = of_iomap(cnp, 0);
		if (!base)
			continue;

		/* Fixup PCIe port DEVICE_ID */
		reg = readl(base + PCIE_VEN_DEV_ID);
		reg = (DEVID_MV78230 << 16) | (reg & 0xffff);
		writel(reg, base + PCIE_VEN_DEV_ID);
	}

	/* Restore SoC Control */
	writel(ctrl, ARMADA_370_XP_SOC_CTRL);

	return 0;
}

static void __noreturn armada_370_xp_restart_soc(struct restart_handler *rst)
{
	writel(0x1, ARMADA_370_XP_SYSCTL_BASE + 0x60);
	writel(0x1, ARMADA_370_XP_SYSCTL_BASE + 0x64);

	hang();
}

static int armada_370_xp_init_soc(void)
{
	u32 reg;

	if (!of_machine_is_compatible("marvell,armada-370-xp"))
		return 0;

	restart_handler_register_fn(armada_370_xp_restart_soc);

	barebox_set_model("Marvell Armada 370/XP");
	barebox_set_hostname("armada");

	/* Disable MBUS error propagation */
	reg = readl(ARMADA_370_XP_FABRIC_CTRL);
	reg &= ~MBUS_ERR_PROP_EN;
	writel(reg, ARMADA_370_XP_FABRIC_CTRL);

	mvebu_mbus_init();

	armada_xp_soc_id_fixup();

	if (of_machine_is_compatible("marvell,armadaxp")) {
		/* Enable GBE0, GBE1, LCD and NFC PUP */
		reg = readl(ARMADA_XP_PUP_ENABLE);
		reg |= GE0_PUP_EN | GE1_PUP_EN | LCD_PUP_EN | NAND_PUP_EN | SPI_PUP_EN;
		writel(reg, ARMADA_XP_PUP_ENABLE);
	}

	return 0;
}
postcore_initcall(armada_370_xp_init_soc);
