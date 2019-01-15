// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe driver for Marvell MVEBU SoCs
 *
 * Based on Linux drivers/pci/host/pci-mvebu.c
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mbus.h>
#include <linux/pci_regs.h>
#include <malloc.h>
#include <of_address.h>
#include <of_gpio.h>
#include <of_pci.h>
#include <linux/sizes.h>

#include "pci-mvebu.h"

/* PCIe unit register offsets */
#define PCIE_DEV_ID_OFF			0x0000
#define PCIE_CMD_OFF			0x0004
#define PCIE_DEV_REV_OFF		0x0008
#define  PCIE_BAR_LO_OFF(n)		(0x0010 + ((n) << 3))
#define  PCIE_BAR_HI_OFF(n)		(0x0014 + ((n) << 3))
#define PCIE_HEADER_LOG_4_OFF		0x0128
#define  PCIE_BAR_CTRL_OFF(n)		(0x1804 + (((n) - 1) * 4))
#define  PCIE_WIN04_CTRL_OFF(n)		(0x1820 + ((n) << 4))
#define  PCIE_WIN04_BASE_OFF(n)		(0x1824 + ((n) << 4))
#define  PCIE_WIN04_REMAP_OFF(n)	(0x182c + ((n) << 4))
#define PCIE_WIN5_CTRL_OFF		0x1880
#define PCIE_WIN5_BASE_OFF		0x1884
#define PCIE_WIN5_REMAP_OFF		0x188c
#define PCIE_CONF_ADDR_OFF		0x18f8
#define  PCIE_CONF_ADDR_EN		BIT(31)
#define  PCIE_CONF_REG(r)		((((r) & 0xf00) << 16) | ((r) & 0xfc))
#define  PCIE_CONF_BUS(b)		(((b) & 0xff) << 16)
#define  PCIE_CONF_DEV(d)		(((d) & 0x1f) << 11)
#define  PCIE_CONF_FUNC(f)		(((f) & 0x7) << 8)
#define  PCIE_CONF_ADDR(bus, devfn, where) \
	(PCIE_CONF_BUS(bus) | PCIE_CONF_DEV(PCI_SLOT(devfn))    | \
	 PCIE_CONF_FUNC(PCI_FUNC(devfn)) | PCIE_CONF_REG(where) | \
	 PCIE_CONF_ADDR_EN)
#define PCIE_CONF_DATA_OFF		0x18fc
#define PCIE_MASK_OFF			0x1910
#define  PCIE_MASK_ENABLE_INTS          (0xf << 24)
#define PCIE_CTRL_OFF			0x1a00
#define  PCIE_CTRL_X1_MODE		BIT(0)
#define PCIE_STAT_OFF			0x1a04
#define  PCIE_STAT_BUS                  (0xff << 8)
#define  PCIE_STAT_DEV                  (0x1f << 16)
#define  PCIE_STAT_LINK_DOWN		BIT(0)
#define PCIE_DEBUG_CTRL         	0x1a60
#define  PCIE_DEBUG_SOFT_RESET		BIT(20)

#define to_pcie(_hc)	container_of(_hc, struct mvebu_pcie, pci)

/*
 * MVEBU PCIe controller needs MEMORY and I/O BARs to be mapped
 * into SoCs address space. Each controller will map 32M of MEM
 * and 64K of I/O space when registered.
 */
static void __iomem *mvebu_pcie_membase = IOMEM(0xe0000000);
static void __iomem *mvebu_pcie_iobase = IOMEM(0xffe00000);

static inline bool mvebu_pcie_link_up(struct mvebu_pcie *pcie)
{
	return !(readl(pcie->base + PCIE_STAT_OFF) & PCIE_STAT_LINK_DOWN);
}

static void mvebu_pcie_set_local_bus_nr(struct pci_controller *host, int busno)
{
	struct mvebu_pcie *pcie = to_pcie(host);
	u32 stat;

	stat = readl(pcie->base + PCIE_STAT_OFF);
	stat &= ~PCIE_STAT_BUS;
	stat |= busno << 8;
	writel(stat, pcie->base + PCIE_STAT_OFF);
}

static void mvebu_pcie_set_local_dev_nr(struct mvebu_pcie *pcie, int devno)
{
	u32 stat;

	stat = readl(pcie->base + PCIE_STAT_OFF);
	stat &= ~PCIE_STAT_DEV;
	stat |= devno << 16;
	writel(stat, pcie->base + PCIE_STAT_OFF);
}

static int mvebu_pcie_indirect_rd_conf(struct pci_bus *bus,
	       unsigned int devfn, int where, int size, u32 *val)
{
	struct mvebu_pcie *pcie = to_pcie(bus->host);

	/* Skip all requests not directed to device behind bridge */
	if (devfn != pcie->devfn || !mvebu_pcie_link_up(pcie)) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	writel(PCIE_CONF_ADDR(bus->number, devfn, where),
	       pcie->base + PCIE_CONF_ADDR_OFF);

	*val = readl(pcie->base + PCIE_CONF_DATA_OFF);

	if (size == 1)
		*val = (*val >> (8 * (where & 3))) & 0xff;
	else if (size == 2)
		*val = (*val >> (8 * (where & 3))) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

static int mvebu_pcie_indirect_wr_conf(struct pci_bus *bus,
	       unsigned int devfn, int where, int size, u32 val)
{
	struct mvebu_pcie *pcie = to_pcie(bus->host);
	u32 _val, shift = 8 * (where & 3);

	/* Skip all requests not directed to device behind bridge */
	if (devfn != pcie->devfn || !mvebu_pcie_link_up(pcie))
		return PCIBIOS_DEVICE_NOT_FOUND;

	writel(PCIE_CONF_ADDR(bus->number, devfn, where),
	       pcie->base + PCIE_CONF_ADDR_OFF);
	_val = readl(pcie->base + PCIE_CONF_DATA_OFF);

	if (size == 4)
		_val = val;
	else if (size == 2)
		_val = (_val & ~(0xffff << shift)) | ((val & 0xffff) << shift);
	else if (size == 1)
		_val = (_val & ~(0xff << shift)) | ((val & 0xff) << shift);
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	writel(_val, pcie->base + PCIE_CONF_DATA_OFF);

	return PCIBIOS_SUCCESSFUL;
}

static int mvebu_pcie_res_start(struct pci_bus *bus, resource_size_t res_addr)
{
	struct mvebu_pcie *pcie = to_pcie(bus->host);

	return (int)pcie->membase + (res_addr & (resource_size(&pcie->mem)-1));
}

static struct pci_ops mvebu_pcie_indirect_ops = {
	.read = mvebu_pcie_indirect_rd_conf,
	.write = mvebu_pcie_indirect_wr_conf,
	.res_start = mvebu_pcie_res_start,
};

/*
 * Setup PCIE BARs and Address Decode Wins:
 * BAR[0,2] -> disabled, BAR[1] -> covers all DRAM banks
 * WIN[0-3] -> DRAM bank[0-3]
 */
static void mvebu_pcie_setup_wins(struct mvebu_pcie *pcie)
{
	const struct mbus_dram_target_info *dram = mvebu_mbus_dram_info();
	u32 size;
	int i;

	/* First, disable and clear BARs and windows. */
	for (i = 1; i < 3; i++) {
		writel(0, pcie->base + PCIE_BAR_CTRL_OFF(i));
		writel(0, pcie->base + PCIE_BAR_LO_OFF(i));
		writel(0, pcie->base + PCIE_BAR_HI_OFF(i));
	}

	for (i = 0; i < 5; i++) {
		writel(0, pcie->base + PCIE_WIN04_CTRL_OFF(i));
		writel(0, pcie->base + PCIE_WIN04_BASE_OFF(i));
		writel(0, pcie->base + PCIE_WIN04_REMAP_OFF(i));
	}

	writel(0, pcie->base + PCIE_WIN5_CTRL_OFF);
	writel(0, pcie->base + PCIE_WIN5_BASE_OFF);
	writel(0, pcie->base + PCIE_WIN5_REMAP_OFF);

	/* Setup windows for DDR banks.  Count total DDR size on the fly. */
	size = 0;
	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel(cs->base & 0xffff0000,
		       pcie->base + PCIE_WIN04_BASE_OFF(i));
		writel(0, pcie->base + PCIE_WIN04_REMAP_OFF(i));
		writel(((cs->size - 1) & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       pcie->base + PCIE_WIN04_CTRL_OFF(i));

		size += cs->size;
	}

	/* Round up 'size' to the nearest power of two. */
	if ((size & (size - 1)) != 0)
		size = 1 << fls(size);

	/* Setup BAR[1] to all DRAM banks. */
	writel(dram->cs[0].base, pcie->base + PCIE_BAR_LO_OFF(1));
	writel(0, pcie->base + PCIE_BAR_HI_OFF(1));
	writel(((size - 1) & 0xffff0000) | 1,
	       pcie->base + PCIE_BAR_CTRL_OFF(1));
}

#define DT_FLAGS_TO_TYPE(flags)		(((flags) >> 24) & 0x03)
#define  DT_TYPE_IO	0x1
#define  DT_TYPE_MEM32	0x2
#define DT_CPUADDR_TO_TARGET(cpuaddr)	(((cpuaddr) >> 56) & 0xFF)
#define DT_CPUADDR_TO_ATTR(cpuaddr)	(((cpuaddr) >> 48) & 0xFF)

static int mvebu_get_target_attr(struct device_node *np, int devfn,
		 unsigned long type, unsigned int *target, unsigned int *attr)
{
	const int na = 3, ns = 2;
	const __be32 *range;
	int rlen, nranges, rangesz, pna, i;

	*target = -1;
	*attr = -1;

	range = of_get_property(np, "ranges", &rlen);
	if (!range)
		return -EINVAL;

	pna = of_n_addr_cells(np);
	rangesz = pna + na + ns;
	nranges = rlen / sizeof(__be32) / rangesz;

	for (i = 0; i < nranges; i++, range += rangesz) {
		u32 flags = of_read_number(range, 1);
		u32 slot = of_read_number(range + 1, 1);
		u64 cpuaddr = of_read_number(range + na, pna);
		unsigned long rtype;

		if (DT_FLAGS_TO_TYPE(flags) == DT_TYPE_IO)
			rtype = IORESOURCE_IO;
		else if (DT_FLAGS_TO_TYPE(flags) == DT_TYPE_MEM32)
			rtype = IORESOURCE_MEM;
		else
			continue;

		if (slot == PCI_SLOT(devfn) && type == rtype) {
			*target = DT_CPUADDR_TO_TARGET(cpuaddr);
			*attr = DT_CPUADDR_TO_ATTR(cpuaddr);
			return 0;
		}
	}

	return -ENOENT;
}

static struct mvebu_pcie *mvebu_pcie_port_probe(struct device_d *dev,
						struct device_node *np)
{
	struct mvebu_pcie *pcie;
	struct clk *clk;
	enum of_gpio_flags flags;
	struct property *prop;
	const __be32 *p;
	int reset_gpio;
	u32 u, port, lane, lane_mask, devfn;
	int mem_target, mem_attr;
	int io_target, io_attr;
	int ret;

	if (of_property_read_u32(np, "marvell,pcie-port", &port)) {
		dev_err(dev, "missing pcie-port property\n");
		return ERR_PTR(-EINVAL);
	}

	lane_mask = 0;
	of_property_for_each_u32(np, "marvell,pcie-lane", prop, p, u)
		lane_mask |= BIT(u);
	lane = ffs(lane_mask)-1;

	devfn = of_pci_get_devfn(np);
	if (devfn < 0) {
		dev_err(dev, "unable to parse devfn\n");
		return ERR_PTR(-EINVAL);
	}

	if (mvebu_get_target_attr(dev->device_node, devfn, IORESOURCE_MEM,
				  &mem_target, &mem_attr)) {
		dev_err(dev, "unable to get target/attr for mem window\n");
		return ERR_PTR(-EINVAL);
	}

	/* I/O windows are optional */
	mvebu_get_target_attr(dev->device_node, devfn, IORESOURCE_IO,
			      &io_target, &io_attr);

	reset_gpio = of_get_named_gpio_flags(np, "reset-gpios", 0, &flags);
	if (gpio_is_valid(reset_gpio)) {
		int reset_active_low = flags & OF_GPIO_ACTIVE_LOW;
		char *reset_name = basprintf("pcie%d.%d-reset", port, lane);
		u32 reset_udelay = 20000;

		of_property_read_u32(np, "reset-delay-us", &reset_udelay);

		ret = gpio_request_one(reset_gpio, GPIOF_DIR_OUT, reset_name);
		if (ret)
			return ERR_PTR(ret);

		/* Ensure a full reset cycle*/
		gpio_set_value(reset_gpio, 1 ^ reset_active_low);
		udelay(reset_udelay);
		gpio_set_value(reset_gpio, 0 ^ reset_active_low);
		udelay(reset_udelay);
	}

	pcie = xzalloc(sizeof(*pcie));
	pcie->port = port;
	pcie->lane = lane;
	pcie->lane_mask = lane_mask;
	pcie->name = basprintf("pcie%d.%d", port, lane);
	pcie->devfn = devfn;

	pcie->base = of_iomap(np, 0);
	if (!pcie->base) {
		dev_err(dev, "PCIe%d.%d unable to map registers\n", port, lane);
		free(pcie);
		return ERR_PTR(-ENOMEM);
	}

	pcie->membase = mvebu_pcie_membase;
	pcie->mem.start = (u32)mvebu_pcie_membase;
	pcie->mem.end = pcie->mem.start + SZ_32M - 1;
	if (mvebu_mbus_add_window_remap_by_id(mem_target, mem_attr,
			      (resource_size_t)pcie->membase, resource_size(&pcie->mem),
			      (u32)pcie->mem.start)) {
		dev_err(dev, "PCIe%d.%d unable to add mbus window for mem at %08x+%08x\n",
			port, lane, (u32)pcie->mem.start, resource_size(&pcie->mem));

		free(pcie);
		return ERR_PTR(-EBUSY);
	}
	mvebu_pcie_membase += SZ_32M;

	if (io_target >= 0 && io_attr >= 0) {
		pcie->iobase = mvebu_pcie_iobase;
		pcie->io.start = (u32)mvebu_pcie_iobase;
		pcie->io.end = pcie->io.start + SZ_64K - 1;

		mvebu_mbus_add_window_remap_by_id(io_target, io_attr,
				  (resource_size_t)pcie->iobase, resource_size(&pcie->io),
				  (u32)pcie->io.start);
		mvebu_pcie_iobase += SZ_64K;
	}

	clk = of_clk_get(np, 0);
	if (!IS_ERR(clk))
		clk_enable(clk);

	pcie->pci.set_busno = mvebu_pcie_set_local_bus_nr;
	pcie->pci.pci_ops = &mvebu_pcie_indirect_ops;
	pcie->pci.mem_resource = &pcie->mem;
	pcie->pci.io_resource = &pcie->io;

	return pcie;
}

static struct mvebu_pcie_ops __maybe_unused armada_370_ops = {
	.phy_setup = armada_370_phy_setup,
};

static struct mvebu_pcie_ops __maybe_unused armada_xp_ops = {
	.phy_setup = armada_xp_phy_setup,
};

static struct of_device_id mvebu_pcie_dt_ids[] = {
#if defined(CONFIG_ARCH_ARMADA_XP)
	{ .compatible = "marvell,armada-xp-pcie", .data = &armada_xp_ops, },
#endif
#if defined(CONFIG_ARCH_ARMADA_370)
	{ .compatible = "marvell,armada-370-pcie", .data = &armada_370_ops, },
#endif
#if defined(CONFIG_ARCH_DOVE)
	{ .compatible = "marvell,dove-pcie", },
#endif
#if defined(CONFIG_ARCH_KIRKWOOD)
	{ .compatible = "marvell,kirkwood-pcie", },
#endif
	{ },
};

static int mvebu_pcie_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	const struct of_device_id *match = of_match_node(mvebu_pcie_dt_ids, np);
	struct mvebu_pcie_ops *ops = (struct mvebu_pcie_ops *)match->data;
	struct device_node *pnp;

	for_each_child_of_node(np, pnp) {
		struct mvebu_pcie *pcie;
		u32 reg;

		if (!of_device_is_available(pnp))
			continue;

		pcie = mvebu_pcie_port_probe(dev, pnp);
		if (IS_ERR(pcie))
			continue;

		if (ops && ops->phy_setup)
			ops->phy_setup(pcie);

		mvebu_pcie_set_local_dev_nr(pcie, 0);
		mvebu_pcie_setup_wins(pcie);

		/* Master + slave enable. */
		reg = readl(pcie->base + PCIE_CMD_OFF);
		reg |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
		reg |= PCI_COMMAND_MASTER;
		writel(reg, pcie->base + PCIE_CMD_OFF);

		/* Disable interrupts */
		reg = readl(pcie->base + PCIE_MASK_OFF);
		reg &= ~PCIE_MASK_ENABLE_INTS;
		writel(reg, pcie->base + PCIE_MASK_OFF);

		register_pci_controller(&pcie->pci);
	}

	return 0;
}

static struct driver_d mvebu_pcie_driver = {
	.name = "mvebu-pcie",
	.probe = mvebu_pcie_probe,
	.of_compatible = mvebu_pcie_dt_ids,
};
device_platform_driver(mvebu_pcie_driver);
