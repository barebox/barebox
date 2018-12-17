// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * based on code
 * Copyright (c) 2010, CompuLab, Ltd.
 * Copyright (c) 2008-2009, NVIDIA Corporation.
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
#include <mach/tegra-powergate.h>
#include <regulator.h>

/* register definitions */

#define AFI_AXI_BAR0_SZ	0x00
#define AFI_AXI_BAR1_SZ	0x04
#define AFI_AXI_BAR2_SZ	0x08
#define AFI_AXI_BAR3_SZ	0x0c
#define AFI_AXI_BAR4_SZ	0x10
#define AFI_AXI_BAR5_SZ	0x14

#define AFI_AXI_BAR0_START	0x18
#define AFI_AXI_BAR1_START	0x1c
#define AFI_AXI_BAR2_START	0x20
#define AFI_AXI_BAR3_START	0x24
#define AFI_AXI_BAR4_START	0x28
#define AFI_AXI_BAR5_START	0x2c

#define AFI_FPCI_BAR0	0x30
#define AFI_FPCI_BAR1	0x34
#define AFI_FPCI_BAR2	0x38
#define AFI_FPCI_BAR3	0x3c
#define AFI_FPCI_BAR4	0x40
#define AFI_FPCI_BAR5	0x44

#define AFI_CACHE_BAR0_SZ	0x48
#define AFI_CACHE_BAR0_ST	0x4c
#define AFI_CACHE_BAR1_SZ	0x50
#define AFI_CACHE_BAR1_ST	0x54

#define AFI_MSI_BAR_SZ		0x60
#define AFI_MSI_FPCI_BAR_ST	0x64
#define AFI_MSI_AXI_BAR_ST	0x68

#define AFI_MSI_VEC0		0x6c
#define AFI_MSI_VEC1		0x70
#define AFI_MSI_VEC2		0x74
#define AFI_MSI_VEC3		0x78
#define AFI_MSI_VEC4		0x7c
#define AFI_MSI_VEC5		0x80
#define AFI_MSI_VEC6		0x84
#define AFI_MSI_VEC7		0x88

#define AFI_MSI_EN_VEC0		0x8c
#define AFI_MSI_EN_VEC1		0x90
#define AFI_MSI_EN_VEC2		0x94
#define AFI_MSI_EN_VEC3		0x98
#define AFI_MSI_EN_VEC4		0x9c
#define AFI_MSI_EN_VEC5		0xa0
#define AFI_MSI_EN_VEC6		0xa4
#define AFI_MSI_EN_VEC7		0xa8

#define AFI_CONFIGURATION		0xac
#define  AFI_CONFIGURATION_EN_FPCI	(1 << 0)

#define AFI_FPCI_ERROR_MASKS	0xb0

#define AFI_INTR_MASK		0xb4
#define  AFI_INTR_MASK_INT_MASK	(1 << 0)
#define  AFI_INTR_MASK_MSI_MASK	(1 << 8)

#define AFI_INTR_CODE			0xb8
#define  AFI_INTR_CODE_MASK		0xf
#define  AFI_INTR_AXI_SLAVE_ERROR	1
#define  AFI_INTR_AXI_DECODE_ERROR	2
#define  AFI_INTR_TARGET_ABORT		3
#define  AFI_INTR_MASTER_ABORT		4
#define  AFI_INTR_INVALID_WRITE		5
#define  AFI_INTR_LEGACY		6
#define  AFI_INTR_FPCI_DECODE_ERROR	7

#define AFI_INTR_SIGNATURE	0xbc
#define AFI_UPPER_FPCI_ADDRESS	0xc0
#define AFI_SM_INTR_ENABLE	0xc4
#define  AFI_SM_INTR_INTA_ASSERT	(1 << 0)
#define  AFI_SM_INTR_INTB_ASSERT	(1 << 1)
#define  AFI_SM_INTR_INTC_ASSERT	(1 << 2)
#define  AFI_SM_INTR_INTD_ASSERT	(1 << 3)
#define  AFI_SM_INTR_INTA_DEASSERT	(1 << 4)
#define  AFI_SM_INTR_INTB_DEASSERT	(1 << 5)
#define  AFI_SM_INTR_INTC_DEASSERT	(1 << 6)
#define  AFI_SM_INTR_INTD_DEASSERT	(1 << 7)

#define AFI_AFI_INTR_ENABLE		0xc8
#define  AFI_INTR_EN_INI_SLVERR		(1 << 0)
#define  AFI_INTR_EN_INI_DECERR		(1 << 1)
#define  AFI_INTR_EN_TGT_SLVERR		(1 << 2)
#define  AFI_INTR_EN_TGT_DECERR		(1 << 3)
#define  AFI_INTR_EN_TGT_WRERR		(1 << 4)
#define  AFI_INTR_EN_DFPCI_DECERR	(1 << 5)
#define  AFI_INTR_EN_AXI_DECERR		(1 << 6)
#define  AFI_INTR_EN_FPCI_TIMEOUT	(1 << 7)
#define  AFI_INTR_EN_PRSNT_SENSE	(1 << 8)

#define AFI_PCIE_CONFIG					0x0f8
#define  AFI_PCIE_CONFIG_PCIE_DISABLE(x)		(1 << ((x) + 1))
#define  AFI_PCIE_CONFIG_PCIE_DISABLE_ALL		0xe
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_MASK	(0xf << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_SINGLE	(0x0 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_420	(0x0 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_X2_X1	(0x0 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_DUAL	(0x1 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_222	(0x1 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_X4_X1	(0x1 << 20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_411	(0x2 << 20)

#define AFI_FUSE			0x104
#define  AFI_FUSE_PCIE_T0_GEN2_DIS	(1 << 2)

#define AFI_PEX0_CTRL			0x110
#define AFI_PEX1_CTRL			0x118
#define AFI_PEX2_CTRL			0x128
#define  AFI_PEX_CTRL_RST		(1 << 0)
#define  AFI_PEX_CTRL_CLKREQ_EN		(1 << 1)
#define  AFI_PEX_CTRL_REFCLK_EN		(1 << 3)
#define  AFI_PEX_CTRL_OVERRIDE_EN	(1 << 4)

#define AFI_PLLE_CONTROL		0x160
#define  AFI_PLLE_CONTROL_BYPASS_PADS2PLLE_CONTROL (1 << 9)
#define  AFI_PLLE_CONTROL_PADS2PLLE_CONTROL_EN (1 << 1)

#define AFI_PEXBIAS_CTRL_0		0x168

#define RP_VEND_XP	0x00000F00
#define  RP_VEND_XP_DL_UP	(1 << 30)

#define RP_PRIV_MISC	0x00000FE0
#define  RP_PRIV_MISC_PRSNT_MAP_EP_PRSNT (0xE << 0)
#define  RP_PRIV_MISC_PRSNT_MAP_EP_ABSNT (0xF << 0)

#define RP_LINK_CONTROL_STATUS			0x00000090
#define  RP_LINK_CONTROL_STATUS_DL_LINK_ACTIVE	0x20000000
#define  RP_LINK_CONTROL_STATUS_LINKSTAT_MASK	0x3fff0000

#define PADS_CTL_SEL		0x0000009C

#define PADS_CTL		0x000000A0
#define  PADS_CTL_IDDQ_1L	(1 << 0)
#define  PADS_CTL_TX_DATA_EN_1L	(1 << 6)
#define  PADS_CTL_RX_DATA_EN_1L	(1 << 10)

#define PADS_PLL_CTL_TEGRA20			0x000000B8
#define PADS_PLL_CTL_TEGRA30			0x000000B4
#define  PADS_PLL_CTL_RST_B4SM			(1 << 1)
#define  PADS_PLL_CTL_LOCKDET			(1 << 8)
#define  PADS_PLL_CTL_REFCLK_MASK		(0x3 << 16)
#define  PADS_PLL_CTL_REFCLK_INTERNAL_CML	(0 << 16)
#define  PADS_PLL_CTL_REFCLK_INTERNAL_CMOS	(1 << 16)
#define  PADS_PLL_CTL_REFCLK_EXTERNAL		(2 << 16)
#define  PADS_PLL_CTL_TXCLKREF_MASK		(0x1 << 20)
#define  PADS_PLL_CTL_TXCLKREF_DIV10		(0 << 20)
#define  PADS_PLL_CTL_TXCLKREF_DIV5		(1 << 20)
#define  PADS_PLL_CTL_TXCLKREF_BUF_EN		(1 << 22)

#define PADS_REFCLK_CFG0			0x000000C8
#define PADS_REFCLK_CFG1			0x000000CC

/*
 * Fields in PADS_REFCLK_CFG*. Those registers form an array of 16-bit
 * entries, one entry per PCIe port. These field definitions and desired
 * values aren't in the TRM, but do come from NVIDIA.
 */
#define PADS_REFCLK_CFG_TERM_SHIFT		2  /* 6:2 */
#define PADS_REFCLK_CFG_E_TERM_SHIFT		7
#define PADS_REFCLK_CFG_PREDI_SHIFT		8  /* 11:8 */
#define PADS_REFCLK_CFG_DRVI_SHIFT		12 /* 15:12 */

/* Default value provided by HW engineering is 0xfa5c */
#define PADS_REFCLK_CFG_VALUE \
	( \
		(0x17 << PADS_REFCLK_CFG_TERM_SHIFT)   | \
		(0    << PADS_REFCLK_CFG_E_TERM_SHIFT) | \
		(0xa  << PADS_REFCLK_CFG_PREDI_SHIFT)  | \
		(0xf  << PADS_REFCLK_CFG_DRVI_SHIFT)     \
	)

/* used to differentiate between Tegra SoC generations */
struct tegra_pcie_soc_data {
	unsigned int num_ports;
	unsigned int msi_base_shift;
	u32 pads_pll_ctl;
	u32 tx_ref_sel;
	bool has_pex_clkreq_en;
	bool has_pex_bias_ctrl;
	bool has_intr_prsnt_sense;
	bool has_avdd_supply;
	bool has_cml_clk;
	bool has_gen2;
};

struct tegra_pcie {
	struct device_d *dev;
	struct pci_controller pci;

	void __iomem *pads;
	void __iomem *afi;
	void __iomem *cs;

	struct list_head buses;

	struct resource io;
	struct resource mem;
	struct resource prefetch;
	struct resource busn;
	struct resource *cs_res;

	struct clk *pex_clk;
	struct clk *afi_clk;
	struct clk *pll_e;
	struct clk *cml_clk;

	struct reset_control *pex_rst;
	struct reset_control *afi_rst;
	struct reset_control *pcie_xrst;

	struct phy *phy;

	struct list_head ports;
	unsigned int num_ports;
	u32 xbar_config;

	struct regulator *pex_clk_supply;
	struct regulator *vdd_supply;
	struct regulator *avdd_supply;

	const struct tegra_pcie_soc_data *soc_data;
};

struct tegra_pcie_port {
	struct tegra_pcie *pcie;
	struct list_head list;
	struct resource regs;
	void __iomem *base;
	unsigned int index;
	unsigned int lanes;
};

static inline struct tegra_pcie *host_to_pcie(struct pci_controller *host)
{
	return container_of(host, struct tegra_pcie, pci);
}

static inline void afi_writel(struct tegra_pcie *pcie, u32 value,
			      unsigned long offset)
{
	writel(value, pcie->afi + offset);
}

static inline u32 afi_readl(struct tegra_pcie *pcie, unsigned long offset)
{
	return readl(pcie->afi + offset);
}

static inline void pads_writel(struct tegra_pcie *pcie, u32 value,
			       unsigned long offset)
{
	writel(value, pcie->pads + offset);
}

static inline u32 pads_readl(struct tegra_pcie *pcie, unsigned long offset)
{
	return readl(pcie->pads + offset);
}

static void __iomem *tegra_pcie_conf_address(struct pci_bus *bus,
					     unsigned int devfn,
					     int where)
{
	struct tegra_pcie *pcie = host_to_pcie(bus->host);
	void __iomem *addr = NULL;

	if (bus->number == 0) {
		unsigned int slot = PCI_SLOT(devfn);
		struct tegra_pcie_port *port;

		list_for_each_entry(port, &pcie->ports, list) {
			if (port->index + 1 == slot) {
				addr = (void __force __iomem *)
					port->regs.start + (where & ~3);
				break;
			}
		}
	} else {
		addr = pcie->cs;

		addr += ((where & 0xf00) << 16) | (bus->number << 16) |
			(PCI_SLOT(devfn) << 11) | (PCI_FUNC(devfn) << 8) |
			(where & 0xfc);
	}

	return addr;
}

static int tegra_pcie_read_conf(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 *value)
{
	void __iomem *addr;

	addr = tegra_pcie_conf_address(bus, devfn, where);
	if (!addr) {
		*value = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	*value = readl(addr);

	if (size == 1)
		*value = (*value >> (8 * (where & 3))) & 0xff;
	else if (size == 2)
		*value = (*value >> (8 * (where & 3))) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

static int tegra_pcie_write_conf(struct pci_bus *bus, unsigned int devfn,
				 int where, int size, u32 value)
{
	void __iomem *addr;
	u32 mask, tmp;

	addr = tegra_pcie_conf_address(bus, devfn, where);
	if (!addr)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (size == 4) {
		writel(value, addr);
		return PCIBIOS_SUCCESSFUL;
	}

	if (size == 2)
		mask = ~(0xffff << ((where & 0x3) * 8));
	else if (size == 1)
		mask = ~(0xff << ((where & 0x3) * 8));
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	tmp = readl(addr) & mask;
	tmp |= value << ((where & 0x3) * 8);
	writel(tmp, addr);

	return PCIBIOS_SUCCESSFUL;
}

static int tegra_pcie_res_start(struct pci_bus *bus, resource_size_t res_addr)
{
	return res_addr;
}

static struct pci_ops tegra_pcie_ops = {
	.read = tegra_pcie_read_conf,
	.write = tegra_pcie_write_conf,
	.res_start = tegra_pcie_res_start,
};

static unsigned long tegra_pcie_port_get_pex_ctrl(struct tegra_pcie_port *port)
{
	unsigned long ret = 0;

	switch (port->index) {
	case 0:
		ret = AFI_PEX0_CTRL;
		break;

	case 1:
		ret = AFI_PEX1_CTRL;
		break;

	case 2:
		ret = AFI_PEX2_CTRL;
		break;
	}

	return ret;
}

static void tegra_pcie_port_reset(struct tegra_pcie_port *port)
{
	unsigned long ctrl = tegra_pcie_port_get_pex_ctrl(port);
	unsigned long value;

	/* pulse reset signal */
	value = afi_readl(port->pcie, ctrl);
	value &= ~AFI_PEX_CTRL_RST;
	afi_writel(port->pcie, value, ctrl);

	mdelay(1);

	value = afi_readl(port->pcie, ctrl);
	value |= AFI_PEX_CTRL_RST;
	afi_writel(port->pcie, value, ctrl);
}

static void tegra_pcie_port_enable(struct tegra_pcie_port *port)
{
	const struct tegra_pcie_soc_data *soc = port->pcie->soc_data;
	unsigned long ctrl = tegra_pcie_port_get_pex_ctrl(port);
	unsigned long value;

	/* enable reference clock */
	value = afi_readl(port->pcie, ctrl);
	value |= AFI_PEX_CTRL_REFCLK_EN;

	if (soc->has_pex_clkreq_en)
		value |= AFI_PEX_CTRL_CLKREQ_EN;

	value |= AFI_PEX_CTRL_OVERRIDE_EN;

	afi_writel(port->pcie, value, ctrl);

	tegra_pcie_port_reset(port);
}

static void tegra_pcie_port_disable(struct tegra_pcie_port *port)
{
	unsigned long ctrl = tegra_pcie_port_get_pex_ctrl(port);
	unsigned long value;

	/* assert port reset */
	value = afi_readl(port->pcie, ctrl);
	value &= ~AFI_PEX_CTRL_RST;
	afi_writel(port->pcie, value, ctrl);

	/* disable reference clock */
	value = afi_readl(port->pcie, ctrl);
	value &= ~AFI_PEX_CTRL_REFCLK_EN;
	afi_writel(port->pcie, value, ctrl);
}

static void tegra_pcie_port_free(struct tegra_pcie_port *port)
{
	list_del(&port->list);
	kfree(port);
}

#if 0
static void tegra_pcie_fixup_bridge(struct pci_dev *dev)
{
	u16 reg;

	if ((dev->class >> 16) == PCI_BASE_CLASS_BRIDGE) {
		pci_read_config_word(dev, PCI_COMMAND, &reg);
		reg |= (PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
			PCI_COMMAND_MASTER | PCI_COMMAND_SERR);
		pci_write_config_word(dev, PCI_COMMAND, reg);
	}
}
DECLARE_PCI_FIXUP_FINAL(PCI_ANY_ID, PCI_ANY_ID, tegra_pcie_fixup_bridge);

/* Tegra PCIE root complex wrongly reports device class */
static void tegra_pcie_fixup_class(struct pci_dev *dev)
{
	dev->class = PCI_CLASS_BRIDGE_PCI << 8;
}
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_NVIDIA, 0x0bf0, tegra_pcie_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_NVIDIA, 0x0bf1, tegra_pcie_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_NVIDIA, 0x0e1c, tegra_pcie_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_NVIDIA, 0x0e1d, tegra_pcie_fixup_class);

/* Tegra PCIE requires relaxed ordering */
static void tegra_pcie_relax_enable(struct pci_dev *dev)
{
	pcie_capability_set_word(dev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_RELAX_EN);
}
DECLARE_PCI_FIXUP_FINAL(PCI_ANY_ID, PCI_ANY_ID, tegra_pcie_relax_enable);
#endif

/*
 * FPCI map is as follows:
 * - 0xfdfc000000: I/O space
 * - 0xfdfe000000: type 0 configuration space
 * - 0xfdff000000: type 1 configuration space
 * - 0xfe00000000: type 0 extended configuration space
 * - 0xfe10000000: type 1 extended configuration space
 */
static void tegra_pcie_setup_translations(struct tegra_pcie *pcie)
{
	u32 fpci_bar, size, axi_address;

	/* Bar 0: type 1 extended configuration space */
	fpci_bar = 0xfe100000;
	size = resource_size(pcie->cs_res);
	axi_address = pcie->cs_res->start;
	afi_writel(pcie, axi_address, AFI_AXI_BAR0_START);
	afi_writel(pcie, size >> 12, AFI_AXI_BAR0_SZ);
	afi_writel(pcie, fpci_bar, AFI_FPCI_BAR0);

	/* Bar 1: downstream IO bar */
	fpci_bar = 0xfdfc0000;
	size = resource_size(&pcie->io);
	axi_address = pcie->io.start;
	afi_writel(pcie, axi_address, AFI_AXI_BAR1_START);
	afi_writel(pcie, size >> 12, AFI_AXI_BAR1_SZ);
	afi_writel(pcie, fpci_bar, AFI_FPCI_BAR1);

	/* Bar 2: prefetchable memory BAR */
	fpci_bar = (((pcie->prefetch.start >> 12) & 0x0fffffff) << 4) | 0x1;
	size = resource_size(&pcie->prefetch);
	axi_address = pcie->prefetch.start;
	afi_writel(pcie, axi_address, AFI_AXI_BAR2_START);
	afi_writel(pcie, size >> 12, AFI_AXI_BAR2_SZ);
	afi_writel(pcie, fpci_bar, AFI_FPCI_BAR2);

	/* Bar 3: non prefetchable memory BAR */
	fpci_bar = (((pcie->mem.start >> 12) & 0x0fffffff) << 4) | 0x1;
	size = resource_size(&pcie->mem);
	axi_address = pcie->mem.start;
	afi_writel(pcie, axi_address, AFI_AXI_BAR3_START);
	afi_writel(pcie, size >> 12, AFI_AXI_BAR3_SZ);
	afi_writel(pcie, fpci_bar, AFI_FPCI_BAR3);

	/* NULL out the remaining BARs as they are not used */
	afi_writel(pcie, 0, AFI_AXI_BAR4_START);
	afi_writel(pcie, 0, AFI_AXI_BAR4_SZ);
	afi_writel(pcie, 0, AFI_FPCI_BAR4);

	afi_writel(pcie, 0, AFI_AXI_BAR5_START);
	afi_writel(pcie, 0, AFI_AXI_BAR5_SZ);
	afi_writel(pcie, 0, AFI_FPCI_BAR5);

	/* map all upstream transactions as uncached */
	/* FIXME: is 0 ok for BAR0 start ??? */
	afi_writel(pcie, 0, AFI_CACHE_BAR0_ST);
	afi_writel(pcie, 0, AFI_CACHE_BAR0_SZ);
	afi_writel(pcie, 0, AFI_CACHE_BAR1_ST);
	afi_writel(pcie, 0, AFI_CACHE_BAR1_SZ);

	/* MSI translations are setup only when needed */
	afi_writel(pcie, 0, AFI_MSI_FPCI_BAR_ST);
	afi_writel(pcie, 0, AFI_MSI_BAR_SZ);
	afi_writel(pcie, 0, AFI_MSI_AXI_BAR_ST);
	afi_writel(pcie, 0, AFI_MSI_BAR_SZ);
}

static int tegra_pcie_phy_enable(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;
	u32 value;

	/* initialize internal PHY, enable up to 16 PCIE lanes */
	pads_writel(pcie, 0x0, PADS_CTL_SEL);

	/* override IDDQ to 1 on all 4 lanes */
	value = pads_readl(pcie, PADS_CTL);
	value |= PADS_CTL_IDDQ_1L;
	pads_writel(pcie, value, PADS_CTL);

	/*
	 * Set up PHY PLL inputs select PLLE output as refclock,
	 * set TX ref sel to div10 (not div5).
	 */
	value = pads_readl(pcie, soc->pads_pll_ctl);
	value &= ~(PADS_PLL_CTL_REFCLK_MASK | PADS_PLL_CTL_TXCLKREF_MASK);
	value |= PADS_PLL_CTL_REFCLK_INTERNAL_CML | soc->tx_ref_sel;
	pads_writel(pcie, value, soc->pads_pll_ctl);

	/* reset PLL */
	value = pads_readl(pcie, soc->pads_pll_ctl);
	value &= ~PADS_PLL_CTL_RST_B4SM;
	pads_writel(pcie, value, soc->pads_pll_ctl);

	udelay(20);

	/* take PLL out of reset  */
	value = pads_readl(pcie, soc->pads_pll_ctl);
	value |= PADS_PLL_CTL_RST_B4SM;
	pads_writel(pcie, value, soc->pads_pll_ctl);

	/* Configure the reference clock driver */
	value = PADS_REFCLK_CFG_VALUE | (PADS_REFCLK_CFG_VALUE << 16);
	pads_writel(pcie, value, PADS_REFCLK_CFG0);
	if (soc->num_ports > 2)
		pads_writel(pcie, PADS_REFCLK_CFG_VALUE, PADS_REFCLK_CFG1);

	/* wait for the PLL to lock */
	if (wait_on_timeout(300 * MSECOND,
		(pads_readl(pcie, soc->pads_pll_ctl) & PADS_PLL_CTL_LOCKDET))) {
		pr_err("Tegra PCIe error: timeout waiting for PLL\n");
		return -EBUSY;
	}

	/* turn off IDDQ override */
	value = pads_readl(pcie, PADS_CTL);
	value &= ~PADS_CTL_IDDQ_1L;
	pads_writel(pcie, value, PADS_CTL);

	/* enable TX/RX data */
	value = pads_readl(pcie, PADS_CTL);
	value |= PADS_CTL_TX_DATA_EN_1L | PADS_CTL_RX_DATA_EN_1L;
	pads_writel(pcie, value, PADS_CTL);

	return 0;
}

static int tegra_pcie_enable_controller(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;
	struct tegra_pcie_port *port;
	unsigned long value;
	int err;

	/* enable PLL power down */
	if (pcie->phy) {
		value = afi_readl(pcie, AFI_PLLE_CONTROL);
		value &= ~AFI_PLLE_CONTROL_BYPASS_PADS2PLLE_CONTROL;
		value |= AFI_PLLE_CONTROL_PADS2PLLE_CONTROL_EN;
		afi_writel(pcie, value, AFI_PLLE_CONTROL);
	}

	/* power down PCIe slot clock bias pad */
	if (soc->has_pex_bias_ctrl)
		afi_writel(pcie, 0, AFI_PEXBIAS_CTRL_0);

	/* configure mode and disable all ports */
	value = afi_readl(pcie, AFI_PCIE_CONFIG);
	value &= ~AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_MASK;
	value |= AFI_PCIE_CONFIG_PCIE_DISABLE_ALL | pcie->xbar_config;

	list_for_each_entry(port, &pcie->ports, list)
		value &= ~AFI_PCIE_CONFIG_PCIE_DISABLE(port->index);

	afi_writel(pcie, value, AFI_PCIE_CONFIG);

	if (soc->has_gen2) {
		value = afi_readl(pcie, AFI_FUSE);
		value &= ~AFI_FUSE_PCIE_T0_GEN2_DIS;
		afi_writel(pcie, value, AFI_FUSE);
	} else {
		value = afi_readl(pcie, AFI_FUSE);
		value |= AFI_FUSE_PCIE_T0_GEN2_DIS;
		afi_writel(pcie, value, AFI_FUSE);
	}

	if (!pcie->phy)
		err = tegra_pcie_phy_enable(pcie);
	else
		err = phy_power_on(pcie->phy);

	/* take the PCIe interface module out of reset */
	reset_control_deassert(pcie->pcie_xrst);

	/* finally enable PCIe */
	value = afi_readl(pcie, AFI_CONFIGURATION);
	value |= AFI_CONFIGURATION_EN_FPCI;
	afi_writel(pcie, value, AFI_CONFIGURATION);

	value = AFI_INTR_EN_INI_SLVERR | AFI_INTR_EN_INI_DECERR |
		AFI_INTR_EN_TGT_SLVERR | AFI_INTR_EN_TGT_DECERR |
		AFI_INTR_EN_TGT_WRERR | AFI_INTR_EN_DFPCI_DECERR;

	if (soc->has_intr_prsnt_sense)
		value |= AFI_INTR_EN_PRSNT_SENSE;

	afi_writel(pcie, value, AFI_AFI_INTR_ENABLE);
	afi_writel(pcie, 0xffffffff, AFI_SM_INTR_ENABLE);

	/* don't enable MSI for now, only when needed */
	afi_writel(pcie, AFI_INTR_MASK_INT_MASK, AFI_INTR_MASK);

	/* disable all exceptions */
	afi_writel(pcie, 0, AFI_FPCI_ERROR_MASKS);

	return 0;
}

static void tegra_pcie_power_off(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;
	int err;

	/* TODO: disable and unprepare clocks? */

	err = phy_power_off(pcie->phy);
	if (err < 0)
		dev_warn(pcie->dev, "failed to power off PHY: %d\n", err);

	reset_control_assert(pcie->pcie_xrst);
	reset_control_assert(pcie->afi_rst);
	reset_control_assert(pcie->pex_rst);

	tegra_powergate_power_off(TEGRA_POWERGATE_PCIE);

	if (soc->has_avdd_supply) {
		err = regulator_disable(pcie->avdd_supply);
		if (err < 0)
			dev_warn(pcie->dev,
				 "failed to disable AVDD regulator: %d\n",
				 err);
	}

	err = regulator_disable(pcie->pex_clk_supply);
	if (err < 0)
		dev_warn(pcie->dev, "failed to disable pex-clk regulator: %d\n",
			 err);

	err = regulator_disable(pcie->vdd_supply);
	if (err < 0)
		dev_warn(pcie->dev, "failed to disable VDD regulator: %d\n",
			 err);
}

static int tegra_pcie_power_on(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;
	int err;

	reset_control_assert(pcie->pcie_xrst);
	reset_control_assert(pcie->afi_rst);
	reset_control_assert(pcie->pex_rst);

	tegra_powergate_power_off(TEGRA_POWERGATE_PCIE);

	/* enable regulators */
	err = regulator_enable(pcie->vdd_supply);
	if (err < 0) {
		dev_err(pcie->dev, "failed to enable VDD regulator: %d\n", err);
		return err;
	}

	err = regulator_enable(pcie->pex_clk_supply);
	if (err < 0) {
		dev_err(pcie->dev, "failed to enable pex-clk regulator: %d\n",
			err);
		return err;
	}

	if (soc->has_avdd_supply) {
		err = regulator_enable(pcie->avdd_supply);
		if (err < 0) {
			dev_err(pcie->dev,
				"failed to enable AVDD regulator: %d\n",
				err);
			return err;
		}
	}

	err = tegra_powergate_sequence_power_up(TEGRA_POWERGATE_PCIE,
						pcie->pex_clk,
						pcie->pex_rst);
	if (err) {
		dev_err(pcie->dev, "powerup sequence failed: %d\n", err);
		return err;
	}

	reset_control_deassert(pcie->afi_rst);

	err = clk_enable(pcie->afi_clk);
	if (err < 0) {
		dev_err(pcie->dev, "failed to enable AFI clock: %d\n", err);
		return err;
	}

	if (soc->has_cml_clk) {
		err = clk_enable(pcie->cml_clk);
		if (err < 0) {
			dev_err(pcie->dev, "failed to enable CML clock: %d\n",
				err);
			return err;
		}
	}

	err = clk_enable(pcie->pll_e);
	if (err < 0) {
		dev_err(pcie->dev, "failed to enable PLLE clock: %d\n", err);
		return err;
	}

	return 0;
}

static int tegra_pcie_clocks_get(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;

	pcie->pex_clk = clk_get(pcie->dev, "pex");
	if (IS_ERR(pcie->pex_clk))
		return PTR_ERR(pcie->pex_clk);

	pcie->afi_clk = clk_get(pcie->dev, "afi");
	if (IS_ERR(pcie->afi_clk))
		return PTR_ERR(pcie->afi_clk);

	pcie->pll_e = clk_get(pcie->dev, "pll_e");
	if (IS_ERR(pcie->pll_e))
		return PTR_ERR(pcie->pll_e);

	if (soc->has_cml_clk) {
		pcie->cml_clk = clk_get(pcie->dev, "cml");
		if (IS_ERR(pcie->cml_clk))
			return PTR_ERR(pcie->cml_clk);
	}

	return 0;
}

static int tegra_pcie_resets_get(struct tegra_pcie *pcie)
{
	pcie->pex_rst = reset_control_get(pcie->dev, "pex");
	if (IS_ERR(pcie->pex_rst))
		return PTR_ERR(pcie->pex_rst);

	pcie->afi_rst = reset_control_get(pcie->dev, "afi");
	if (IS_ERR(pcie->afi_rst))
		return PTR_ERR(pcie->afi_rst);

	pcie->pcie_xrst = reset_control_get(pcie->dev, "pcie_x");
	if (IS_ERR(pcie->pcie_xrst))
		return PTR_ERR(pcie->pcie_xrst);

	return 0;
}

static int tegra_pcie_get_resources(struct tegra_pcie *pcie)
{
	struct device_d *dev = pcie->dev;
	int err;

	err = tegra_pcie_clocks_get(pcie);
	if (err) {
		dev_err(dev, "failed to get clocks: %d\n", err);
		return err;
	}

	err = tegra_pcie_resets_get(pcie);
	if (err) {
		dev_err(dev, "failed to get resets: %d\n", err);
		return err;
	}

	pcie->phy = phy_optional_get(pcie->dev, "pcie");
	if (IS_ERR(pcie->phy)) {
		err = PTR_ERR(pcie->phy);
		dev_err(dev, "failed to get PHY: %d\n", err);
		return err;
	}

	err = phy_init(pcie->phy);
	if (err < 0) {
		dev_err(dev, "failed to initialize PHY: %d\n", err);
		return err;
	}

	err = tegra_pcie_power_on(pcie);
	if (err) {
		dev_err(dev, "failed to power up: %d\n", err);
		return err;
	}

	pcie->pads = dev_request_mem_region_by_name(dev, "pads");
	if (IS_ERR(pcie->pads)) {
		err = PTR_ERR(pcie->pads);
		goto poweroff;
	}

	pcie->afi = dev_request_mem_region_by_name(dev, "afi");
	if (IS_ERR(pcie->afi)) {
		err = PTR_ERR(pcie->afi);
		goto poweroff;
	}

	pcie->cs_res = dev_get_resource_by_name(dev, IORESOURCE_MEM, "cs");
	pcie->cs = dev_request_mem_region_by_name(dev, "cs");
	if (IS_ERR(pcie->cs)) {
		err = PTR_ERR(pcie->cs);
		goto poweroff;
	}

	return 0;

poweroff:
	tegra_pcie_power_off(pcie);
	return err;
}

static int tegra_pcie_put_resources(struct tegra_pcie *pcie)
{
	int err;

	tegra_pcie_power_off(pcie);

	err = phy_exit(pcie->phy);
	if (err < 0)
		dev_err(pcie->dev, "failed to teardown PHY: %d\n", err);

	return 0;
}

static int tegra_pcie_get_xbar_config(struct tegra_pcie *pcie, u32 lanes,
				      u32 *xbar)
{
	struct device_node *np = pcie->dev->device_node;

	if (of_device_is_compatible(np, "nvidia,tegra124-pcie")) {
		switch (lanes) {
		case 0x0000104:
			dev_info(pcie->dev, "4x1, 1x1 configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_X4_X1;
			return 0;

		case 0x0000102:
			dev_info(pcie->dev, "2x1, 1x1 configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_X2_X1;
			return 0;
		}
	} else if (of_device_is_compatible(np, "nvidia,tegra30-pcie")) {
		switch (lanes) {
		case 0x00000204:
			dev_info(pcie->dev, "4x1, 2x1 configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_420;
			return 0;

		case 0x00020202:
			dev_info(pcie->dev, "2x3 configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_222;
			return 0;

		case 0x00010104:
			dev_info(pcie->dev, "4x1, 1x2 configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_411;
			return 0;
		}
	} else if (of_device_is_compatible(np, "nvidia,tegra20-pcie")) {
		switch (lanes) {
		case 0x00000004:
			dev_info(pcie->dev, "single-mode configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_SINGLE;
			return 0;

		case 0x00000202:
			dev_info(pcie->dev, "dual-mode configuration\n");
			*xbar = AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_DUAL;
			return 0;
		}
	}

	return -EINVAL;
}

static int tegra_pcie_parse_dt(struct tegra_pcie *pcie)
{
	const struct tegra_pcie_soc_data *soc = pcie->soc_data;
	struct device_node *np = pcie->dev->device_node, *port;
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource *rp_res;
	struct resource res;
	u32 lanes = 0;
	int err;

	if (of_pci_range_parser_init(&parser, np)) {
		dev_err(pcie->dev, "missing \"ranges\" property\n");
		return -EINVAL;
	}

	pcie->vdd_supply = regulator_get(pcie->dev, "vdd");
	if (IS_ERR(pcie->vdd_supply))
		return PTR_ERR(pcie->vdd_supply);

	pcie->pex_clk_supply = regulator_get(pcie->dev, "pex-clk");
	if (IS_ERR(pcie->pex_clk_supply))
		return PTR_ERR(pcie->pex_clk_supply);

	if (soc->has_avdd_supply) {
		pcie->avdd_supply = regulator_get(pcie->dev, "avdd");
		if (IS_ERR(pcie->avdd_supply))
			return PTR_ERR(pcie->avdd_supply);
	}

	for_each_of_pci_range(&parser, &range) {
		of_pci_range_to_resource(&range, np, &res);

		switch (res.flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			memcpy(&pcie->io, &res, sizeof(res));
			pcie->io.name = "I/O";
			break;

		case IORESOURCE_MEM:
			if (res.flags & IORESOURCE_PREFETCH) {
				memcpy(&pcie->prefetch, &res, sizeof(res));
				pcie->prefetch.name = "PREFETCH";
			} else {
				memcpy(&pcie->mem, &res, sizeof(res));
				pcie->mem.name = "MEM";
			}
			break;
		}
	}
#if 0
	err = of_pci_parse_bus_range(np, &pcie->busn);
	if (err < 0) {
		dev_err(pcie->dev, "failed to parse ranges property: %d\n",
			err);
		pcie->busn.name = np->name;
		pcie->busn.start = 0;
		pcie->busn.end = 0xff;
		pcie->busn.flags = IORESOURCE_BUS;
	}
#endif
	/* parse root ports */
	for_each_child_of_node(np, port) {
		struct tegra_pcie_port *rp;
		unsigned int index;
		u32 value;

		err = of_pci_get_devfn(port);
		if (err < 0) {
			dev_err(pcie->dev, "failed to parse address: %d\n",
				err);
			return err;
		}

		index = PCI_SLOT(err);

		if (index < 1 || index > soc->num_ports) {
			dev_err(pcie->dev, "invalid port number: %d\n", index);
			return -EINVAL;
		}

		index--;

		err = of_property_read_u32(port, "nvidia,num-lanes", &value);
		if (err < 0) {
			dev_err(pcie->dev, "failed to parse # of lanes: %d\n",
				err);
			return err;
		}

		if (value > 16) {
			dev_err(pcie->dev, "invalid # of lanes: %u\n", value);
			return -EINVAL;
		}

		lanes |= value << (index << 3);

		if (!of_device_is_available(port))
			continue;

		rp = kzalloc(sizeof(*rp), GFP_KERNEL);
		if (!rp)
			return -ENOMEM;

		err = of_address_to_resource(port, 0, &rp->regs);
		if (err < 0) {
			dev_err(pcie->dev, "failed to parse address: %d\n",
				err);
			return err;
		}

		/*
		 * if the port address is inside the NULL page we alias this
		 * range at +1MB and fix up the resource, otherwise just request
		 */
		if (!(rp->regs.start & 0xfffff000)) {
			rp_res = request_iomem_region(dev_name(pcie->dev),
					rp->regs.start + SZ_1M,
					rp->regs.end + SZ_1M);
			if (!rp_res)
				return -ENOMEM;

			map_io_sections(rp->regs.start,
					(void *)rp_res->start, SZ_1M);

			rp->regs.start = rp_res->start;
			rp->regs.end = rp_res->end;
		} else {
			rp_res = request_iomem_region(dev_name(pcie->dev),
					rp->regs.start, rp->regs.end);
			if (!rp_res)
				return -ENOMEM;
		}

		INIT_LIST_HEAD(&rp->list);
		rp->index = index;
		rp->lanes = value;
		rp->pcie = pcie;

		rp->base = (void __force __iomem *)rp->regs.start;
		if (IS_ERR(rp->base))
			return PTR_ERR(rp->base);

		list_add_tail(&rp->list, &pcie->ports);
	}

	err = tegra_pcie_get_xbar_config(pcie, lanes, &pcie->xbar_config);
	if (err < 0) {
		dev_err(pcie->dev, "invalid lane configuration\n");
		return err;
	}

	return 0;
}

/*
 * FIXME: If there are no PCIe cards attached, then calling this function
 * can result in the increase of the bootup time as there are big timeout
 * loops.
 */
#define TEGRA_PCIE_LINKUP_TIMEOUT	200	/* up to 1.2 seconds */
static bool tegra_pcie_port_check_link(struct tegra_pcie_port *port)
{
	unsigned int timeout;
	unsigned int retries = 2;
	u32 value;

	/* override presence detection */
	value = readl(port->base + RP_PRIV_MISC);
	value &= ~RP_PRIV_MISC_PRSNT_MAP_EP_ABSNT;
	value |= RP_PRIV_MISC_PRSNT_MAP_EP_PRSNT;
	writel(value, port->base + RP_PRIV_MISC);

	do {
		timeout = wait_on_timeout(150 * MSECOND,
			readl(port->regs.start + RP_VEND_XP) & RP_VEND_XP_DL_UP);

		if (timeout) {
			dev_dbg(port->pcie->dev, "link %u down, retrying\n",
				port->index);
			goto retry;
		}

		timeout = wait_on_timeout(200 * MSECOND,
			readl(port->regs.start + RP_LINK_CONTROL_STATUS) &
			RP_LINK_CONTROL_STATUS_DL_LINK_ACTIVE);

		if (!timeout)
			return true;

retry:
		tegra_pcie_port_reset(port);
	} while (--retries);

	return false;
}

static int tegra_pcie_enable(struct tegra_pcie *pcie)
{
	struct tegra_pcie_port *port, *tmp;

	list_for_each_entry_safe(port, tmp, &pcie->ports, list) {
		dev_dbg(pcie->dev, "probing port %u, using %u lanes\n",
			 port->index, port->lanes);

		tegra_pcie_port_enable(port);

		if (tegra_pcie_port_check_link(port))
			continue;

		dev_dbg(pcie->dev, "link %u down, ignoring\n", port->index);

		tegra_pcie_port_disable(port);
		tegra_pcie_port_free(port);
	}

	pcie->pci.parent = pcie->dev;
	pcie->pci.pci_ops = &tegra_pcie_ops;
	pcie->pci.mem_resource = &pcie->mem;
	pcie->pci.mem_pref_resource = &pcie->prefetch;
	pcie->pci.io_resource = &pcie->io;

	register_pci_controller(&pcie->pci);

	return 0;
}

static const struct tegra_pcie_soc_data tegra20_pcie_data = {
	.num_ports = 2,
	.msi_base_shift = 0,
	.pads_pll_ctl = PADS_PLL_CTL_TEGRA20,
	.tx_ref_sel = PADS_PLL_CTL_TXCLKREF_DIV10,
	.has_pex_clkreq_en = false,
	.has_pex_bias_ctrl = false,
	.has_intr_prsnt_sense = false,
	.has_avdd_supply = false,
	.has_cml_clk = false,
	.has_gen2 = false,
};

static const struct tegra_pcie_soc_data tegra30_pcie_data = {
	.num_ports = 3,
	.msi_base_shift = 8,
	.pads_pll_ctl = PADS_PLL_CTL_TEGRA30,
	.tx_ref_sel = PADS_PLL_CTL_TXCLKREF_BUF_EN,
	.has_pex_clkreq_en = true,
	.has_pex_bias_ctrl = true,
	.has_intr_prsnt_sense = true,
	.has_avdd_supply = true,
	.has_cml_clk = true,
	.has_gen2 = false,
};

static const struct tegra_pcie_soc_data tegra124_pcie_data = {
	.num_ports = 2,
	.msi_base_shift = 8,
	.pads_pll_ctl = PADS_PLL_CTL_TEGRA30,
	.tx_ref_sel = PADS_PLL_CTL_TXCLKREF_BUF_EN,
	.has_pex_clkreq_en = true,
	.has_pex_bias_ctrl = true,
	.has_intr_prsnt_sense = true,
	.has_cml_clk = true,
	.has_gen2 = true,
};

static __maybe_unused struct of_device_id tegra_pcie_of_match[] = {
	{
		.compatible = "nvidia,tegra124-pcie",
		.data = &tegra124_pcie_data
	}, {
		.compatible = "nvidia,tegra30-pcie",
		.data = &tegra30_pcie_data
	}, {
		.compatible = "nvidia,tegra20-pcie",
		.data = &tegra20_pcie_data
	}, {
		/* sentinel */
	},
};

static int tegra_pcie_probe(struct device_d *dev)
{
	struct tegra_pcie *pcie;
	int err;

	pcie = kzalloc(sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	INIT_LIST_HEAD(&pcie->buses);
	INIT_LIST_HEAD(&pcie->ports);
	dev_get_drvdata(dev, (const void **)&pcie->soc_data);
	pcie->dev = dev;

	err = tegra_pcie_parse_dt(pcie);
	if (err < 0) {
		printk("parse DT failed\n");
		return err;
	}

	err = tegra_pcie_get_resources(pcie);
	if (err < 0) {
		dev_err(dev, "failed to request resources: %d\n", err);
		return err;
	}

	err = tegra_pcie_enable_controller(pcie);
	if (err)
		goto put_resources;

	/* setup the AFI address translations */
	tegra_pcie_setup_translations(pcie);

	err = tegra_pcie_enable(pcie);
	if (err < 0) {
		dev_err(dev, "failed to enable PCIe ports: %d\n", err);
		goto put_resources;
	}

	return 0;

put_resources:
	tegra_pcie_put_resources(pcie);
	return err;
}

static struct driver_d tegra_pcie_driver = {
	.name = "tegra-pcie",
	.of_compatible = DRV_OF_COMPAT(tegra_pcie_of_match),
	.probe = tegra_pcie_probe,
};
device_platform_driver(tegra_pcie_driver);
