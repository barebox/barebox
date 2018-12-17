/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Synopsys DesignWare PCIe host controller driver
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jingoo Han <jg1.han@samsung.com>
 */

#ifndef _PCIE_DESIGNWARE_H
#define _PCIE_DESIGNWARE_H

/* Parameters for the waiting for link up routine */
#define LINK_WAIT_MAX_RETRIES		10
#define LINK_WAIT_USLEEP_MAX		100000

/* Parameters for the waiting for iATU enabled routine */
#define LINK_WAIT_MAX_IATU_RETRIES     5
#define LINK_WAIT_IATU_MAX             10000

/* Synopsis specific PCIE configuration registers */
#define PCIE_PORT_LINK_CONTROL		0x710
#define PORT_LINK_MODE_MASK		(0x3f << 16)
#define PORT_LINK_MODE_1_LANES		(0x1 << 16)
#define PORT_LINK_MODE_2_LANES		(0x3 << 16)
#define PORT_LINK_MODE_4_LANES		(0x7 << 16)

#define PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define PORT_LOGIC_SPEED_CHANGE		(0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK	(0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES	(0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES	(0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES	(0x4 << 8)

#define PCIE_MSI_ADDR_LO		0x820
#define PCIE_MSI_ADDR_HI		0x824
#define PCIE_MSI_INTR0_ENABLE		0x828
#define PCIE_MSI_INTR0_MASK		0x82C
#define PCIE_MSI_INTR0_STATUS		0x830

#define PCIE_ATU_VIEWPORT		0x900
#define PCIE_ATU_REGION_INBOUND		(0x1 << 31)
#define PCIE_ATU_REGION_OUTBOUND	(0x0 << 31)
#define PCIE_ATU_REGION_INDEX2		(0x2 << 0)
#define PCIE_ATU_REGION_INDEX1		(0x1 << 0)
#define PCIE_ATU_REGION_INDEX0		(0x0 << 0)
#define PCIE_ATU_CR1			0x904
#define PCIE_ATU_TYPE_MEM		(0x0 << 0)
#define PCIE_ATU_TYPE_IO		(0x2 << 0)
#define PCIE_ATU_TYPE_CFG0		(0x4 << 0)
#define PCIE_ATU_TYPE_CFG1		(0x5 << 0)
#define PCIE_ATU_CR2			0x908
#define PCIE_ATU_ENABLE			(0x1 << 31)
#define PCIE_ATU_BAR_MODE_ENABLE	(0x1 << 30)
#define PCIE_ATU_LOWER_BASE		0x90C
#define PCIE_ATU_UPPER_BASE		0x910
#define PCIE_ATU_LIMIT			0x914
#define PCIE_ATU_LOWER_TARGET		0x918
#define PCIE_ATU_BUS(x)			(((x) & 0xff) << 24)
#define PCIE_ATU_DEV(x)			(((x) & 0x1f) << 19)
#define PCIE_ATU_FUNC(x)		(((x) & 0x7) << 16)
#define PCIE_ATU_UPPER_TARGET		0x91C

/*
 * iATU Unroll-specific register definitions
 * From 4.80 core version the address translation will be made by unroll
 */
#define PCIE_ATU_UNR_REGION_CTRL1	0x00
#define PCIE_ATU_UNR_REGION_CTRL2	0x04
#define PCIE_ATU_UNR_LOWER_BASE	0x08
#define PCIE_ATU_UNR_UPPER_BASE	0x0C
#define PCIE_ATU_UNR_LIMIT		0x10
#define PCIE_ATU_UNR_LOWER_TARGET	0x14
#define PCIE_ATU_UNR_UPPER_TARGET	0x18

/* Register address builder */
#define PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(region)  ((0x3 << 20) | (region << 9))

/* PCIe Port Logic registers */
#define PLR_OFFSET                     0x700
#define PCIE_PHY_DEBUG_R1              (PLR_OFFSET + 0x2c)
#define PCIE_PHY_DEBUG_R1_LINK_UP      (0x1 << 4)
#define PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING     (0x1 << 29)

#define PCIE_MISC_CONTROL_1_OFF		0x8BC
#define PCIE_DBI_RO_WR_EN		(0x1 << 0)

/* PCIe Port Logic registers */
#define PLR_OFFSET                     0x700
#define PCIE_PHY_DEBUG_R1              (PLR_OFFSET + 0x2c)
#define PCIE_PHY_DEBUG_R1_LINK_UP      (0x1 << 4)
#define PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING     (0x1 << 29)

struct pcie_port;
struct dw_pcie;

struct dw_pcie_host_ops {
	int (*rd_own_conf)(struct pcie_port *pp, int where, int size, u32 *val);
	int (*wr_own_conf)(struct pcie_port *pp, int where, int size, u32 val);
	int (*rd_other_conf)(struct pcie_port *pp, struct pci_bus *bus,
			     unsigned int devfn, int where, int size, u32 *val);
	int (*wr_other_conf)(struct pcie_port *pp, struct pci_bus *bus,
			     unsigned int devfn, int where, int size, u32 val);
	int (*host_init)(struct pcie_port *pp);
	void (*scan_bus)(struct pcie_port *pp);
};

struct pcie_port {
	u8			root_bus_nr;
	u64			cfg0_base;
	u64			cfg0_mod_base;
	void __iomem		*va_cfg0_base;
	u32			cfg0_size;
	u64			cfg1_base;
	u64			cfg1_mod_base;
	void __iomem		*va_cfg1_base;
	u32			cfg1_size;
	u64			io_base;
	u64			io_mod_base;
	phys_addr_t		io_bus_addr;
	u32			io_size;
	u64			mem_base;
	u64			mem_mod_base;
	phys_addr_t		mem_bus_addr;
	u32			mem_size;
	struct resource		cfg;
	struct resource		io;
	struct resource		mem;
	struct resource		busn;
	int			irq;
	const struct dw_pcie_host_ops	*ops;
	struct pci_controller	pci;
};

struct dw_pcie_ops {
	u32     (*readl_dbi)(struct dw_pcie *pcie, void __iomem *base, u32 reg,
			     size_t size);
	void    (*writel_dbi)(struct dw_pcie *pcie, void __iomem *base, u32 reg,
			      size_t size, u32 val);
	int     (*link_up)(struct dw_pcie *pcie);
};

struct dw_pcie {
	struct device_d         *dev;
	void __iomem            *dbi_base;
	u32                     num_viewport;
	u8                      iatu_unroll_enabled;
	struct pcie_port        pp;
	const struct dw_pcie_ops *ops;
};

#define to_dw_pcie_from_pp(port) container_of((port), struct dw_pcie, pp)

int dw_pcie_read(void __iomem *addr, int size, u32 *val);
int dw_pcie_write(void __iomem *addr, int size, u32 val);
void dw_pcie_setup_rc(struct pcie_port *pp);
int dw_pcie_host_init(struct pcie_port *pp);

u32 __dw_pcie_readl_dbi(struct dw_pcie *pci, void __iomem *addr, u32 reg,
			size_t size);
void __dw_pcie_writel_dbi(struct dw_pcie *pci, void __iomem *addr, u32 reg,
			  size_t size, u32 val);
int dw_pcie_link_up(struct dw_pcie *pci);
int dw_pcie_wait_for_link(struct dw_pcie *pci);
void dw_pcie_prog_outbound_atu(struct dw_pcie *pci, int index,
			       int type, u64 cpu_addr, u64 pci_addr,
			       u32 size);
void dw_pcie_setup(struct dw_pcie *pci);

static inline void dw_pcie_writel_dbi(struct dw_pcie *pci, u32 reg, u32 val)
{
	__dw_pcie_writel_dbi(pci, pci->dbi_base, reg, 0x4, val);
}

static inline u32 dw_pcie_readl_dbi(struct dw_pcie *pci, u32 reg)
{
	return  __dw_pcie_readl_dbi(pci, pci->dbi_base, reg, 0x4);
}

static inline void dw_pcie_dbi_ro_wr_en(struct dw_pcie *pci)
{
	u32 reg;
	u32 val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = dw_pcie_readl_dbi(pci, reg);
	val |= PCIE_DBI_RO_WR_EN;
	dw_pcie_writel_dbi(pci, reg, val);
}

static inline void dw_pcie_dbi_ro_wr_dis(struct dw_pcie *pci)
{
	u32 reg;
	u32 val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = dw_pcie_readl_dbi(pci, reg);
	val &= ~PCIE_DBI_RO_WR_EN;
	dw_pcie_writel_dbi(pci, reg, val);
}

#endif /* _PCIE_DESIGNWARE_H */
